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

#ifndef __YATOS_IRQ_H
#define __YATOS_IRQ_H

#include <arch/irq.h>
#include <arch/regs.h>
#include <yatos/list.h>


struct irq_action
{
  void *private_data;
  void (*action)(void *private_data, struct pt_regs * context);
  struct list_head list;
};

struct irq_slot
{
  struct list_head action_list;
  uint32 flag;
  uint32 count;
};

void irq_init();
int  irq_regist(int irq_num, struct irq_action * action);
void irq_unregist(int irq_num, struct irq_action *action);
void irq_action_init(struct  irq_action * action);

#endif /* __YATOS_IRQ_H */
