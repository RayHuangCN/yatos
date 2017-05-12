/*************************************************
 *   Author: Ray Huang
 *   Date  : 2017/4/13
 *   Email : rayhuang@126.com
 *   Desc  : start file
 ************************************************/
#include <unistd.h>
#include "sys_call.h"
#include <sys/wait.h>
#include <sys/stat.h>
#include <errno.h>

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
  return sys_call_4(SYS_CALL_EXECVE, path, argv, envp);
}

int usleep(__useconds_t __useconds)
{
  return sys_call_2(SYS_CALL_USLEEP, __useconds);
}

pid_t waitpid(__pid_t __pid,int* __stat_loc,int __options)
{
  return sys_call_4(SYS_CALL_WAITPID, __pid, __stat_loc, __options);
}

pid_t getpid()
{
  return sys_call_1(SYS_CALL_GETPID);
}

void *sbrk(intptr_t increment)
{
  return (void *)sys_call_2(SYS_CALL_SBRK, increment);
}

int brk(void *addr)
{
  return sys_call_2(SYS_CALL_BRK, addr);
}

int chdir(const char* __path)
{
  return sys_call_2(SYS_CALL_CHDIR, __path);
}

int mkdir(const char * name, mode_t mode)
{
  return sys_call_3(SYS_CALL_MKDIR, name, mode);
}
int rmdir(const char* __path)
{
  return sys_call_2(SYS_CALL_RMDIR, __path);
}

void sync()
{
  sys_call_1(SYS_CALL_FSSYNC);
}

int link(const char* __from,const char* __to)
{
  return sys_call_3(SYS_CALL_LINK, __from, __to);
}

int unlink(const char* __name)
{
  return sys_call_2(SYS_CALL_UNLINK, __name);
}

int pipe(int __pipedes[])
{
  return sys_call_2(SYS_CALL_PIPE, __pipedes);
}
