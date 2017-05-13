#ifndef __MYGLIB_SYSCALL_H
#define __MYGLIB_SYSCALL_H

/*************************************************
 *   Author: Ray Huang
 *   Date  : 2017/4/20
 *   Email : rayhuang110@126.com
 *   Desc  : syscall low level
 ************************************************/
#include <errno.h>

#define SYS_CALL_FORK   1
#define SYS_CALL_EXIT   2
#define SYS_CALL_EXECVE 3
#define SYS_CALL_WAITPID 4
#define SYS_CALL_SBRK  5
#define SYS_CALL_BRK  6
#define SYS_CALL_GETPID 7
#define SYS_CALL_CHDIR 8

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

#define SYS_CALL_USLEEP 30

#define SYS_CALL_PIPE 40
#define SYS_CALL_SIGNAL 41
#define SYS_CALL_SIGRET 42
#define SYS_CALL_SIGPROCMASK 43
#define SYS_CALL_KILL 44
/* asm functions */
int __sys_call_1(unsigned long call_num);
int __sys_call_2(unsigned long call_num, unsigned long arg1);
int __sys_call_3(unsigned long call_num,
                         unsigned long arg1, unsigned long arg2);
int __sys_call_4(unsigned long call_num,
                         unsigned long arg1, unsigned long arg2,
                         unsigned long arg3);




#define sys_call_1(sys_call_num) sys_call_1_c((unsigned long)sys_call_num)
#define sys_call_2(sys_call_num, arg1) sys_call_2_c((unsigned long)sys_call_num, (unsigned long)(arg1))
#define sys_call_3(sys_call_num, arg1, arg2) sys_call_3_c((unsigned long)sys_call_num, (unsigned long)(arg1), (unsigned long)(arg2))
#define sys_call_4(sys_call_num, arg1, arg2, arg3) sys_call_4_c((unsigned long)sys_call_num, (unsigned long)(arg1), (unsigned long)(arg2), (unsigned long)(arg3))

/* c functions */
int sys_call_1_c(unsigned long call_num);
int sys_call_2_c(unsigned long call_num,
                 unsigned long arg1);
int sys_call_3_c(unsigned long call_num,
                 unsigned long arg1,
                 unsigned long arg2);
int sys_call_4_c(unsigned long call_num,
                 unsigned long arg1,
                 unsigned long arg2,
                 unsigned long arg3);

#endif
