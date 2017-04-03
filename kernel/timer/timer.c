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

int time = 1;
/********* g_function ***************************/
void timer_irq_handler(void * private, struct irq_context *irq_context)
{
  time++;
  if (time == TIMER_HZ){
    printk("hand timer irq\n\r");
    time = 0;
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
  return 0;
}

void timer_unregister(struct timer_action *action)
{
}
