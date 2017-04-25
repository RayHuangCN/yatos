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

static struct irq_slot irq_slots[IRQ_TOTAL_NUM];


/********* g_function ***************************/

void default_irq_handler(struct pt_regs irq_info)
{
  struct irq_slot * target_slot = irq_slots + irq_info.irq_num;
  struct list_head * pos = NULL;
  struct irq_action * cur_action = NULL;

  list_for_each(pos, &(target_slot->action_list)){
    cur_action = list_entry(pos, struct irq_action, list);
    if (cur_action->action)
      cur_action->action(cur_action->private_data, &irq_info);
  }

  arch_irq_ack();
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

int irq_regist(int irq_num, struct irq_action * action)
{
  struct irq_slot * target_slot = irq_slots + irq_num;

  uint32 irq_save = arch_irq_save();
  arch_irq_disable();

  list_add_tail(&(action->list), &(target_slot->action_list));

  arch_irq_recover(irq_save);

  return 0;
}

void  irq_unregist(int irq_num,struct irq_action* action)
{

  struct irq_slot * target_slot = irq_slots + irq_num;
  struct list_head * pos = NULL;
  uint32 irq_save = arch_irq_save();
  arch_irq_disable();

  list_for_each(pos, &(target_slot->action_list))
    if (pos == &(action->list)){
      list_del(pos);
      break;
    }

  arch_irq_recover(irq_save);

}

void irq_action_init(struct irq_action * action)
{
  action->action = NULL;
  action->private_data = NULL;
  INIT_LIST_HEAD(&(action->list));
}
