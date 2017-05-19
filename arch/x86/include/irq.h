/*
 *  IRQ lowleve operations
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

#ifndef __ARCH_IRQ_H
#define __ARCH_IRQ_H

#include <arch/regs.h>

#define IRQ_PAGE_FAULT 14
#define IRQ_SYSCALL 0x80
#define IRQ_TIMER  0x20
#define IRQ_TOTAL_NUM 256

#define IRQ_8259A_VEC_START 0x20
#define IRQ_8259A_VEC_NUM   8

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

typedef void (* irq_handler)(struct pt_regs);

void arch_irq_enable(void);
void arch_irq_disable(void);
void arch_irq_init(irq_handler default_handler);
void arch_irq_set_handler(int irq_num, irq_handler handler);
void arch_irq_ack();
uint32 arch_irq_save();
void arch_irq_recover(uint32 saved);

#endif /* __ARCH_IRQ_H */
