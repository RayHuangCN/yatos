#ifndef __ARCH_TASK_H
#define __ARCH_TASK_H

/*************************************************
 *   Author: Ray Huang
 *   Date  : 2017/4/17
 *   Email : rayhuang@126.com
 *   Desc  : low level task manager interface
 ************************************************/
#include <arch/system.h>

void task_arch_launch(unsigned long start_addr, unsigned long stack);

#endif
