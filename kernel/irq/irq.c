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


static uint32 irq_deep = 1;
static struct irq_slot irq_slots[IRQ_TOTAL_NUM];
/********* g_function ***************************/
void default_irq_handler(struct irq_info irq_info, struct regs regs)
{
  printk("irq handled num = %d\n\r", irq_info.irq_num);
}

void irq_init()
{
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
