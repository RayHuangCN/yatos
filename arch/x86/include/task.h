#ifndef __ARCH_TASK_H
#define __ARCH_TASK_H

/*************************************************
 *   Author: Ray Huang
 *   Date  : 2017/4/17
 *   Email : rayhuang@126.com
 *   Desc  : low level task manager interface
 ************************************************/
#include <arch/system.h>
#include <yatos/task.h>
#include <arch/regs.h>
struct  task_sche_frame
{
  uint32 esi;
  uint32 edi;
  uint32 ebp;
  uint32 eax;
  uint32 ebx;
  uint32 ecx;
  uint32 edx;
  uint32 eflags;
  uint32 eip;
};


void task_arch_launch(unsigned long start_addr, unsigned long stack);
void task_arch_init();
void task_arch_befor_launch(struct task * task);
void task_arch_init_run_context(struct task * task, unsigned long ret_val);
void task_arch_switch_to(struct task * pre, struct task *next);

#endif
