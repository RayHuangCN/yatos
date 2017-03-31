#ifndef __IRQ_H
#define __IRQ_H

/*************************************************
 *   Author: Ray Huang
 *   Date  : 2017/3/31
 *   Email : rayhuang@126.com
 *   Desc  : irq lowleve interface
 ************************************************/
#include <arch/regs.h>

#define IRQ_SYSCALL 128

#define IRQ_TOTAL_NUM 256

#define INTR_TYPE 6
#define TRAP_TYPE 7

#define irq_init_idt_entry(num, type, DPL) \
  do{\
    extern void irq_num_##num();\
    uint32 irq_handler =(uint32) irq_num_##num;    \
    uint32 low = 0, high = 0;\
    low = (irq_handler & 0x0000ffff) | (GDT_KERNEL_CS << 16);\
    high = (irq_handler & 0xffff0000) | (type << 8) | (1 << 11) | (DPL << 13) | (1 << 15);\
    uint32 *idt_base = (uint32*)IDT_BASE;\
    idt_base[num << 1] = low;            \
    idt_base[(num<<1) + 1] = high;\
  }while(0)\





struct irq_info
{
  uint32 irq_num;
  uint32 err_code;
};



typedef void (* irq_handler)(struct irq_info , struct regs);
/********* g_function ***************************/
void arch_irq_enable(void);
void arch_irq_disable(void);

void arch_irq_init(irq_handler default_handler);
void arch_irq_set_handler(int irq_num, irq_handler handler);

#endif
