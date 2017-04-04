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

/********* g_define *****************************/

/********* g_variable ***************************/
static struct list_head action_list;
static struct irq_action timer_irq_ac;
static unsigned long timer_click;
/********* g_function ***************************/
void timer_irq_handler(void * private, struct irq_context *irq_context)
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

void timer_init()
{
  arch_timer_init(TIMER_HZ);

  INIT_LIST_HEAD(&action_list);
  irq_action_init(&timer_irq_ac);
  timer_irq_ac.action = timer_irq_handler;
  irq_regist(TIMER_IRQ_NUM, &timer_irq_ac);
}

int timer_register(struct timer_action *action)
{
  struct list_head  * cur = NULL;
  struct timer_action * cur_action = NULL;
  irq_disable();

  list_for_each(cur, &action_list){
    cur_action = container_of(cur, struct timer_action, list_entry);
    if (cur_action->target_click > action->target_click)
      break;
  }
  if (cur == &action_list)
    list_add_tail(&(action->list_entry), &action_list);
  else
    list_add(&(action->list_entry), cur->prev);

  irq_enable();
  return 0;
}

void timer_unregister(struct timer_action *action)
{
  struct list_head * cur = NULL;
  struct timer_action * cur_action = NULL;
  irq_disable();

  list_for_each(cur, &action_list){
    cur_action = container_of(cur, struct timer_action, list_entry);
    if (cur_action == action){
      list_del(cur);
      break;
    }
  }

  irq_enable();
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
