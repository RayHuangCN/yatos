/*************************************************
 *   Author: Ray Huang
 *   Date  : 2017/4/13
 *   Email : rayhuang@126.com
 *   Desc  : start file
 ************************************************/
#include <unistd.h>
#include "sys_call.h"

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
  return sys_call_4(SYS_CALL_EXECVE, (unsigned long)path, (unsigned long)argv
             , (unsigned long)envp);
}
