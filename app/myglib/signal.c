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

int sigaction(int num, const struct sigaction * act, struct sigaction * oldact)
{
  return sys_call_4(SYS_CALL_SIGNAL, num, act, oldact);
}

int sigprocmask(int __how,const sigset_t* __set,sigset_t* __oset)
{
  return sys_call_4(SYS_CALL_SIGPROCMASK, __how, __set, __oset);
}

__sighandler_t signal(int __sig,__sighandler_t __handler)
{
  __sighandler_t oldhandler;
  struct sigaction old;
  int ret;
  ret = sigaction(__sig, NULL, &old);
  if (ret < 0)
    return SIG_ERR;

  oldhandler = old.sa_handler;
  old.sa_handler = __handler;

  ret = sigaction(__sig, &old, NULL);
  if (ret < 0)
    return SIG_ERR;
  return oldhandler;
}

int kill(__pid_t __pid,int __sig)
{
  return sys_call_3(SYS_CALL_KILL, __pid, __sig);
}

int sigismember(const sigset_t* __set,int __signo)
{
  return __set->__val[0] & (1 << __signo);
}

int sigemptyset(sigset_t* __set)
{
  __set->__val[0] = 0;
  return 0;
}
int sigfillset(sigset_t* __set)
{
  __set->__val[0] = ~0;
  return 0;
}

int sigaddset(sigset_t* __set,int __signo)
{
  __set->__val[0] |= 1 << __signo;
  return 0;
}

int sigdelset(sigset_t* __set,int __signo)
{
  __set->__val[0] &= ~(1 << __signo);
  return 0;
}

int sigandset(sigset_t* __set,const sigset_t* __left,const sigset_t* __right)
{
  __set->__val[0]  = __left->__val[0] & __right->__val[0];
  return 0;
}

int sigorset(sigset_t* __set,const sigset_t* __left,const sigset_t* __right)
{
  __set->__val[0] = __left->__val[0] | __right->__val[0];
  return 0;
}

int sigisemptyset(const sigset_t* __set)
{
  return __set->__val[0] == 0;
}
