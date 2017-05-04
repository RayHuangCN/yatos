/*************************************************
 *   Author: Ray Huang
 *   Date  : 2017/4/13
 *   Email : rayhuang@126.com
 *   Desc  : start file
 ************************************************/
#include <unistd.h>
#include "sys_call.h"
#include <sys/wait.h>
pid_t fork()
{
  return sys_call_1(SYS_CALL_FORK);
}

void _exit(int status)
{
  sys_call_2(SYS_CALL_EXIT, status);
}

int execve(const char* path,char* const argv[],char* const envp[])
{
  return sys_call_4(SYS_CALL_EXECVE,
                    (unsigned long)path,
                    (unsigned long)argv
                    , (unsigned long)envp);
}

int usleep(__useconds_t __useconds)
{
  return sys_call_2(SYS_CALL_USLEEP, __useconds);
}

pid_t waitpid(__pid_t __pid,int* __stat_loc,int __options)
{
  return sys_call_4(SYS_CALL_WAITPID,
                    (unsigned long)__pid,
                    (unsigned long)__stat_loc,
                    (unsigned long)__options);
}


pid_t getpid()
{
  return sys_call_1(SYS_CALL_GETPID);
}

void *sbrk(intptr_t increment)
{
  return (void *)sys_call_2(SYS_CALL_SBRK,
                    (unsigned long)increment);
}

int brk(void *addr)
{
  return sys_call_2(SYS_CALL_BRK,
                    (unsigned long)addr);
}

int chdir(const char* __path)
{
  return sys_call_2(SYS_CALL_CHDIR,
                    (unsigned long)__path);
}
