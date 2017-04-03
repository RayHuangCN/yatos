#ifndef __YATOS_IRQ_H
#define __YATOS_IRQ_H

/*************************************************
 *   Author: Ray Huang
 *   Date  : 2017/3/31
 *   Email : rayhuang@126.com
 *   Desc  : common irq interface
 ************************************************/

/********* header files *************************/
#include <arch/irq.h>
#include <yatos/list.h>
/********* g_define *****************************/
struct irq_action
{
  void *private_data;
  void (*action)(void *private_data, struct irq_context * context);
  struct list_head list;
};
struct irq_slot
{
  struct list_head action_list;
  uint32 flag;
  uint32 count;
};

/********* g_variable ***************************/
/********* g_function ***************************/
void irq_disable();
void irq_enable();
void irq_init();

int  irq_regist(int irq_num, struct irq_action * action);
void irq_unregist(int irq_num, struct irq_action *action);
int irq_action_init(struct  irq_action * action);


#endif
