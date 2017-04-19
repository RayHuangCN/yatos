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
void task_arch_launch(unsigned long start_addr, unsigned long stack);
void task_arch_init();
void task_arch_befor_launch(struct task * task);

#endif
