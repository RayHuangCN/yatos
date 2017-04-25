#ifndef __YATOS_SYS_CALL_H
#define __YATOS_SYS_CALL_H

/*************************************************
 *   Author: Ray Huang
 *   Date  : 2017/4/20
 *   Email : rayhuang@126.com
 *   Desc  : syscall
 ************************************************/
#include <yatos/irq.h>

//task manager
#define SYS_CALL_FORK 1
#define SYS_CALL_EXIT 2
#define SYS_CALL_EXECVE 3
#define SYS_CALL_WAITPID 4

//file operation
#define SYS_CALL_OPEN 10
#define SYS_CALL_READ 11
#define SYS_CALL_WRITE 12
#define SYS_CALL_SEEK 13
#define SYS_CALL_CLOSE 14
#define SYS_CALL_SYNC 15

//timer
#define SYS_CALL_USLEEP 30

#define SYS_CALL_MAX_NNUM 256

typedef int (*sys_call_fun)(struct pt_regs * regs);

void sys_call_init();
void sys_call_regist(int sys_call_num, sys_call_fun do_sys_call);
void sys_call_unregist(int sys_call_num);

#endif
