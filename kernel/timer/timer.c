/*************************************************
 *   Author: Ray Huang
 *   Date  : 2017/4/2
 *   Email : rayhuang@126.com
 *   Desc  : timer
 ************************************************/

/********* header files *************************/
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
    if (cur_action->action)
      cur_action->action(cur_action->private);
  }

}

int timer_register(struct timer_action *action)
{
  struct list_head  * cur = NULL;
  struct timer_action * cur_action = NULL;
  uint32 is = arch_irq_save();
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

  arch_irq_recover(is);
  return 0;
}

void timer_unregister(struct timer_action *action)
{
  struct list_head * cur = NULL;
  struct timer_action * cur_action = NULL;
  uint32 is = arch_irq_save();
  arch_irq_disable();

  list_for_each(cur, &action_list){
    cur_action = container_of(cur, struct timer_action, list_entry);
    if (cur_action == action){
      list_del(cur);
      break;
    }
  }

  arch_irq_recover(is);
}


void timer_action_init(struct timer_action* action)
{
  action->action = NULL;
  INIT_LIST_HEAD(&(action->list_entry));
  action->target_click = 0;
}

unsigned long timer_get_click()
{
  return timer_click;
}

static void timer_usleep_action(void *private)
{
  struct task * task = (struct task *)private;
  task_ready_to_run(task);
}

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

static int sys_call_usleep(struct pt_regs * regs)
{
  unsigned long usec = (unsigned long)sys_call_arg1(regs);
  return timer_usleep(usec);
}

void timer_init()
{
  arch_timer_init(TIMER_HZ);

  INIT_LIST_HEAD(&action_list);
  irq_action_init(&timer_irq_ac);
  timer_irq_ac.action = timer_irq_handler;
  irq_regist(TIMER_IRQ_NUM, &timer_irq_ac);
  sys_call_regist(SYS_CALL_USLEEP, sys_call_usleep);
}
