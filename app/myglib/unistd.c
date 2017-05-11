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
  int ret;
  ret = sys_call_1(SYS_CALL_FORK);
  if (ret  < 0){
    errno = -ret;
    return -1;
  }
  return ret;
}

void _exit(int status)
{

  sys_call_2(SYS_CALL_EXIT, status);

}

int execve(const char* path,char* const argv[],char* const envp[])
{
  int ret;
  ret = sys_call_4(SYS_CALL_EXECVE,
                    (unsigned long)path,
                    (unsigned long)argv
                    , (unsigned long)envp);
  if (ret  < 0){
    errno = -ret;
    return -1;
  }
  return ret;
}

int usleep(__useconds_t __useconds)
{
  int ret;
  ret = sys_call_2(SYS_CALL_USLEEP, __useconds);
  if (ret  < 0){
    errno = -ret;
    return -1;
  }
  return ret;
}

pid_t waitpid(__pid_t __pid,int* __stat_loc,int __options)
{
  int ret;
  ret = sys_call_4(SYS_CALL_WAITPID,
                    (unsigned long)__pid,
                    (unsigned long)__stat_loc,
                    (unsigned long)__options);
  if (ret  < 0){
    errno = -ret;
    return -1;
  }
  return ret;
}


pid_t getpid()
{
  int ret;
  ret = sys_call_1(SYS_CALL_GETPID);
  if (ret  < 0){
    errno = -ret;
    return -1;
  }
  return ret;
}

void *sbrk(intptr_t increment)
{
  return (void *)sys_call_2(SYS_CALL_SBRK,
                    (unsigned long)increment);
}

int brk(void *addr)
{
  int ret;
  ret = sys_call_2(SYS_CALL_BRK,
                    (unsigned long)addr);
  if (ret  < 0){
    errno = -ret;
    return -1;
  }
  return ret;
}

int chdir(const char* __path)
{
  int ret;
  ret = sys_call_2(SYS_CALL_CHDIR,
                    (unsigned long)__path);
  if (ret  < 0){
    errno = -ret;
    return -1;
  }
  return ret;
}

int mkdir(const char * name, mode_t mode)
{
  int ret;
  ret = sys_call_3(SYS_CALL_MKDIR,
                    (unsigned long)name,
                    (unsigned long)mode);
  if (ret  < 0){
    errno = -ret;
    return -1;
  }
  return ret;
}
int rmdir(const char* __path)
{
  int ret;
  ret = sys_call_2(SYS_CALL_RMDIR,
                    (unsigned long)__path);
  if (ret  < 0){
    errno = -ret;
    return -1;
  }
  return ret;
}

void sync()
{
  errno = -sys_call_1(SYS_CALL_FSSYNC);
}


int link(const char* __from,const char* __to)
{
  int ret;
  ret = sys_call_3(SYS_CALL_LINK,
                    (unsigned long)__from,
                    (unsigned long)__to);
  if (ret  < 0){
    errno = -ret;
    return -1;
  }
  return ret;
}

int unlink(const char* __name)
{
  int ret;
  ret = sys_call_2(SYS_CALL_UNLINK,
                    (unsigned long)__name);
  if (ret  < 0){
    errno = -ret;
    return -1;
  }
  return ret;
}

int pipe(int __pipedes[])
{
  int ret;
  ret = sys_call_2(SYS_CALL_PIPE,
                   (unsigned long)__pipedes);
  if (ret < 0){
    errno = -ret;
    return -1;
  }
  return ret;
}
