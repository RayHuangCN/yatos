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
#define SYS_CALL_SBRK 5
#define SYS_CALL_BRK 6
#define SYS_CALL_GETPID 7
#define SYS_CALL_CHDIR 8

//file operation
#define SYS_CALL_OPEN 10
#define SYS_CALL_READ 11
#define SYS_CALL_WRITE 12
#define SYS_CALL_SEEK 13
#define SYS_CALL_CLOSE 14
#define SYS_CALL_SYNC 15
#define SYS_CALL_IOCTL 16
#define SYS_CALL_READDIR 17
#define SYS_CALL_MKDIR 18
#define SYS_CALL_LINK 19
#define SYS_CALL_UNLINK 20
#define SYS_CALL_RMDIR 21
#define SYS_CALL_FTRUNCATE 22
#define SYS_CALL_FSSYNC 23
#define SYS_CALL_FSTAT 24
#define SYS_CALL_DUP3 25
#define SYS_CALL_FCNTL 26

//timer
#define SYS_CALL_USLEEP 30

//ipc
#define SYS_CALL_PIPE 40

#define SYS_CALL_MAX_NNUM 256

typedef int (*sys_call_fun)(struct pt_regs * regs);

void sys_call_init();
void sys_call_regist(int sys_call_num, sys_call_fun do_sys_call);
void sys_call_unregist(int sys_call_num);

#endif
