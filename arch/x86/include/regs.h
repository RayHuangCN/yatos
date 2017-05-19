/*
 *  Regs define
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

#ifndef __ARCH_REGS_H
#define __ARCH_REGS_H

#include <arch/system.h>

#define sys_call_num(regs) (regs->eax)
#define sys_call_ret(regs) (regs->eax)
#define sys_call_arg1(regs) (regs->ebx)
#define sys_call_arg2(regs) (regs->ecx)
#define sys_call_arg3(regs) (regs->edx)
#define pt_regs_user_stack(regs) (regs->esp)
#define pt_regs_ret_addr(regs) (regs->eip)
typedef uint32 regs_type;

struct pt_regs
{
  uint32 ebx;
  uint32 ecx;
  uint32 edx;
  uint32 esi;
  uint32 edi;
  uint32 ebp;
  uint32 eax;
  uint32 ds;
  uint32 es;
  uint32 irq_num;
  uint32 erro_code;
  uint32 eip;
  uint32 cs;
  uint32 eflags;
  uint32 esp;
  uint32 ss;
};


struct tss
{
  uint32 pre_tss;
  uint32 esp0;
  uint32 ss0;
  uint32 esp1;
  uint32 ss1;
  uint32 esp2;
  uint32 ss2;
  uint32 cr3;
  uint32 eip;
  uint32 eflags;
  uint32 eax;
  uint32 ecx;
  uint32 edx;
  uint32 ebx;
  uint32 esp;
  uint32 ebp;
  uint32 esi;
  uint32 edi;
  uint32 es;
  uint32 cs;
  uint32 ss;
  uint32 ds;
  uint32 fs;
  uint32 gs;
  uint32 ldt;
  uint32 io;
};

#endif /* __ARCH_REGS_H */
