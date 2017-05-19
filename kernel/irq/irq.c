/*
 *  Irq management.
 *
 *  Copyright (C) 2017 ese@ccnt.zju
 *
 *  ---------------------------------------------------
 *  Started at 2017/3/31 by Ray
 *
 *  ---------------------------------------------------
 *
 *  This file is subject to the terms and conditions of the GNU General Public
 *  License.
 */

#include <arch/irq.h>
#include <arch/system.h>
#include <yatos/printk.h>
#include <yatos/irq.h>
#include <yatos/list.h>
#include <yatos/tools.h>

static struct irq_slot irq_slots[IRQ_TOTAL_NUM];
/*
 * This is the common function of all irq.
 * This function will be called from irq low level asm code.
 */
void default_irq_handler(struct pt_regs irq_info)
{
  struct irq_slot * target_slot;
  struct list_head * pos;;
  struct irq_action * cur_action;;

  target_slot = irq_slots + irq_info.irq_num;
  list_for_each(pos, &(target_slot->action_list)){
    cur_action = list_entry(pos, struct irq_action, list);
    if (cur_action->action)
      cur_action->action(cur_action->private_data, &irq_info);
  }

  arch_irq_ack();
}

/*
 * Initate irq management.
 */
void irq_init()
{
  int i;
  for (i = 0; i < IRQ_TOTAL_NUM; i++)
    INIT_LIST_HEAD(&(irq_slots[i].action_list));
  arch_irq_init(default_irq_handler);
}

/*
 * Register a irq action.
 * All the actions with the same irq number will be linked as a list.
 * All the actions with the same irq number will be called when that irq is handled.
 * Actions only be deleted when irq_unregist is called.
 * Return 0 if successful or return -1 if any error.
 */
int irq_regist(int irq_num, struct irq_action * action)
{
  struct irq_slot * target_slot;
  uint32 irq_save;

  target_slot = irq_slots + irq_num;
  irq_save = arch_irq_save();

  arch_irq_disable();
  list_add_tail(&(action->list), &(target_slot->action_list));

  arch_irq_recover(irq_save);

  return 0;
}

/*
 * Unregist a irq action.
 */
void  irq_unregist(int irq_num,struct irq_action* action)
{
  uint32 irq_save = arch_irq_save();
  arch_irq_disable();
  list_del(&(action->list));
  arch_irq_recover(irq_save);
}

/*
 * A function help to initate "action".
 */
void irq_action_init(struct irq_action * action)
{
  action->action = NULL;
  action->private_data = NULL;
  INIT_LIST_HEAD(&(action->list));
}
