/*
 *  timer
 *
 *  Copyright (C) 2017 ese@ccnt.zju
 *
 *  ---------------------------------------------------
 *  Started at 2017/4/2 by Ray
 *
 *  ---------------------------------------------------
 *
 *  This file is subject to the terms and conditions of the GNU General Public
 *  License.
 */

#include <yatos/irq.h>
#include <yatos/printk.h>
#include <yatos/list.h>
#include <yatos/timer.h>
#include <arch/timer.h>
#include <yatos/sys_call.h>
#include <yatos/schedule.h>
#include <yatos/errno.h>
#include <yatos/signal.h>

static struct list_head action_list;
static struct irq_action timer_irq_ac;
static unsigned long timer_click;

/*
 * Irq action function of timer irq.
 * Timeout timer actions will be called in this function.
 *
 * Note: once a timer action be called, it will be deleted,
 * So, if a timer action want to be periodic, it must regist self when it being called.
 */
void timer_irq_handler(void * private, struct pt_regs *irq_context)
{
  struct list_head * cur = NULL;
  struct timer_action * cur_action = NULL;
  struct list_head * temp  = NULL;

  timer_click++;
  list_for_each_safe(cur, temp, &action_list){
    cur_action = container_of(cur, struct timer_action, list_entry);
    if (cur_action->target_click > timer_click)
      break;

    list_del(cur);
    //this is safe for timer_unregister
    INIT_LIST_HEAD(cur);

    if (cur_action->action)
      cur_action->action(cur_action->private);
  }
}

/*
 * Register a timer action.
 * All actions will be linked no-descending.
 */
int timer_register(struct timer_action *action)
{
  struct list_head  * cur;
  struct timer_action * cur_action;
  uint32 irq_save = arch_irq_save();

  arch_irq_disable();
  list_for_each(cur, &action_list){
    cur_action = container_of(cur, struct timer_action, list_entry);
    if (cur_action->target_click > action->target_click)
      break;
  }
  if (cur == &action_list)
    list_add_tail(&(action->list_entry), &action_list);
  else
    list_add(&(action->list_entry), cur->prev);
  arch_irq_recover(irq_save);

  return 0;
}

/*
 * Unregist a timer action.
 * If "action" is in action_list now, it is safe to delete it.
 * If "action" is expired and had been removed from action_list, it is also safe to delete it
 * since we call INIT_LIST_HEAD once we delete action.
 * see  more details in timer_irq_handler.
 */
void timer_unregister(struct timer_action *action)
{
  uint32 is = arch_irq_save();
  arch_irq_disable();
  list_del(&(action->list_entry));
  arch_irq_recover(is);
}

/*
 * For init timer action.
 */
void timer_action_init(struct timer_action* action)
{
  action->action = NULL;
  INIT_LIST_HEAD(&(action->list_entry));
  action->target_click = 0;
  action->private = NULL;
}

/*
 * Get current click of timer.
 * Click will be increase one a timer irq come.
 * 1 click = 1 / TIMER_HZ (ms)
 */
unsigned long timer_get_click()
{
  return timer_click;
}

/*
 * The timer action of timer_usleep
 * Just let target task to wakeup
 */
static void timer_usleep_action(void *private)
{
  struct task * task = (struct task *)private;
  task_ready_to_run(task);
}

/*
 * Block current task for "usec"(us).
 *
 * Note: this function will return -EINTR when a signal recived.
 */
int timer_usleep(unsigned long usec)
{
  struct timer_action action;
  struct task * task = task_get_cur();

  timer_action_init(&action);
  action.target_click = timer_click + (usec * TIMER_HZ / 1000000);
  action.private  = task;
  action.action = timer_usleep_action;

  task_block(task);
  timer_register(&action);
  task_schedule();
  timer_unregister(&action);
  if (sig_is_pending(task))
    return -EINTR;
  return 0;
}

/*
 * usleep system call.
 */
static int sys_call_usleep(struct pt_regs * regs)
{
  unsigned long usec = (unsigned long)sys_call_arg1(regs);
  return timer_usleep(usec);
}

/*
 * Initate timer management.
 */
void timer_init()
{
  arch_timer_init(TIMER_HZ);
  INIT_LIST_HEAD(&action_list);
  irq_action_init(&timer_irq_ac);
  timer_irq_ac.action = timer_irq_handler;
  irq_regist(TIMER_IRQ_NUM, &timer_irq_ac);
  sys_call_regist(SYS_CALL_USLEEP, sys_call_usleep);
}
