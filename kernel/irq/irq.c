/*************************************************
 *   Author: Ray Huang
 *   Date  : 2017/3/31
 *   Email : rayhuang@126.com
 *   Desc  : common irq system.
 ************************************************/

/********* header files *************************/
#include <arch/irq.h>
#include <yatos/printk.h>
#include <yatos/irq.h>
#include <yatos/list.h>
#include <arch/system.h>
#include <yatos/tools.h>

static uint32 irq_deep = 1;
static struct irq_slot irq_slots[IRQ_TOTAL_NUM];


/********* g_function ***************************/

void default_irq_handler(struct irq_context irq_info)
{
  irq_disable();
  if (irq_info.irq_num == 14)
  printk("%d irq\n", irq_info.irq_num);

  struct irq_slot * target_slot = irq_slots + irq_info.irq_num;
  struct list_head * pos = NULL;
  struct irq_action * cur_action = NULL;

  list_for_each(pos, &(target_slot->action_list)){
    cur_action = list_entry(pos, struct irq_action, list);
    if (cur_action->action)
      cur_action->action(cur_action->private_data, &irq_info);
  }

  arch_irq_ack();
  irq_enable();
 }

void irq_init()
{
  int i;
  for (i = 0; i < IRQ_TOTAL_NUM; i++) {
    INIT_LIST_HEAD(&(irq_slots[i].action_list));
    irq_slots[i].count = 0;
    irq_slots[i].flag = 0;
  }
  arch_irq_init(default_irq_handler);

}

void irq_disable()
{
  if (irq_deep == 0)
    arch_irq_disable();
  irq_deep++;
}

void irq_enable()
{
  if (irq_deep == 1)
    arch_irq_enable();
  irq_deep--;
}

int irq_regist(int irq_num, struct irq_action * action)
{
  struct irq_slot * target_slot = irq_slots + irq_num;

  irq_disable();

  list_add_tail(&(target_slot->action_list), &(action->list));

  irq_enable();

  return 0;
}

void  irq_unregist(int irq_num,struct irq_action* action)
{

  struct irq_slot * target_slot = irq_slots + irq_num;
  struct list_head * pos = NULL;
  irq_disable();

  list_for_each(pos, &(target_slot->action_list))
    if (pos == &(action->list)){
      list_del(pos);
      break;
    }

  irq_enable();

}

int irq_action_init(struct irq_action * action)
{
  action->action = NULL;
  action->private_data = NULL;
  INIT_LIST_HEAD(&(action->list));
}
