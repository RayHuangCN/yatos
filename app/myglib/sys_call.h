#ifndef __MYGLIB_SYSCALL_H
#define __MYGLIB_SYSCALL_H

/*************************************************
 *   Author: Ray Huang
 *   Date  : 2017/4/20
 *   Email : rayhuang110@126.com
 *   Desc  : syscall low level
 ************************************************/


#define SYS_CALL_FORK   1
#define SYS_CALL_EXIT   2
#define SYS_CALL_EXECVE 3
#define SYS_CALL_WAITPID 4

#define SYS_CALL_OPEN 10
#define SYS_CALL_READ 11
#define SYS_CALL_WRITE 12
#define SYS_CALL_SEEK 13
#define SYS_CALL_CLOSE 14
#define SYS_CALL_SYNC 15

#define SYS_CALL_USLEEP 30

int sys_call_1(unsigned long call_num);

int sys_call_2(unsigned long call_num, unsigned long arg1);

int sys_call_3(unsigned long call_num,
                         unsigned long arg1, unsigned long arg2);

int sys_call_4(unsigned long call_num,
                         unsigned long arg1, unsigned long arg2,
                         unsigned long arg3);




#endif
