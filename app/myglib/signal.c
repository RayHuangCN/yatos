/*************************************************
 *   Author: Ray Huang
 *   Date  : 2017/5/12
 *   Email : rayhuang@126.com
 *   Desc  : signal
 ************************************************/
#include <stdio.h>
#include <unistd.h>
#include <error.h>
#include <signal.h>
#include "sys_call.h"

int sigaction(int num, const struct sigaction * act, struct sigaction * oldaction)
{
  return 0;
}

int sigprocmask(int __how,const sigset_t* __set,sigset_t* __oset)
{
  int ret;
  ret = sys_call_4(SYS_CALL_SIGPROCMASK,
                   (unsigned long)__how,
                   (unsigned long)__set,
                   (unsigned long)__oset);
  if (ret < 0){
    errno = -ret;
    return -1;
  }
  return errno = 0;
}

__sighandler_t signal(int __sig,__sighandler_t __handler)
{
  return __handler;
}
