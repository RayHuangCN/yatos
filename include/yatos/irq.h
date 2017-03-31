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
/********* g_define *****************************/
struct irq_operation
{
};
struct irq_slot
{
};

/********* g_variable ***************************/
/********* g_function ***************************/
void irq_disable();
void irq_enable();
void irq_init();

#endif
