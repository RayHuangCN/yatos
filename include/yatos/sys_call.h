#ifndef __YATOS_SYS_CALL_H
#define __YATOS_SYS_CALL_H

/*************************************************
 *   Author: Ray Huang
 *   Date  : 2017/4/20
 *   Email : rayhuang@126.com
 *   Desc  : syscall
 ************************************************/
#include <yatos/irq.h>

#define SYS_CALL_FORK 1
#define SYS_CALL_EXIT 2
#define SYS_CALL_MAX_NNUM 128

typedef int (*sys_call_fun)(struct pt_regs * regs);
void sys_call_init();
void sys_call_regist(int sys_call_num, sys_call_fun do_sys_call);
void sys_call_unregist(int sys_call_num);

#endif
