#ifndef __YATOS_SIGNAL_H
#define __YATOS_SIGNAL_H

/*************************************************
 *   Author: Ray Huang
 *   Date  : 2017/5/12
 *   Email : rayhuang@126.com
 *   Desc  : signal
 ************************************************/
#include <yatos/task.h>
#include <yatos/list.h>

typedef void (*sig_handler)(int);

#define NSIG		32
#define SIG_DFL	((sig_handler)0)	/* default signal handling */
#define SIG_IGN	((sig_handler)1)	/* ignore signal */
#define SIG_ERR	((sig_handler)-1)	/* error return from signal */

#define SIGHUP		 1
#define SIGINT		 2
#define SIGQUIT		 3
#define SIGILL		 4
#define SIGTRAP		 5
#define SIGABRT		 6
#define SIGIOT		 6
#define SIGBUS		 7
#define SIGFPE		 8
#define SIGKILL		 9
#define SIGUSR1		10
#define SIGSEGV		11
#define SIGUSR2		12
#define SIGPIPE		13
#define SIGALRM		14
#define SIGTERM		15
#define SIGSTKFLT	16
#define SIGCHLD		17
#define SIGCONT		18
#define SIGSTOP		19
#define SIGTSTP		20
#define SIGTTIN		21
#define SIGTTOU		22
#define SIGURG		23
#define SIGXCPU		24
#define SIGXFSZ		25
#define SIGVTALRM	26
#define SIGPROF		27
#define SIGWINCH	28
#define SIGIO		29
#define SIGPOLL		SIGIO
/*
  #define SIGLOST		29
*/
#define SIGPWR		30
#define SIGSYS		31
#define	SIGUNUSED	31

/* These should not be considered constants from userland.  */
#define SIGRTMIN	32
#define SIGRTMAX	_NSIG

/* for siprocmask */
#define SIG_BLOCK 0
#define SIG_UNBLOCK 1
#define SIG_SET 2

/* default signal action */
#define SIG_ACTION_EXIT 1
#define SIG_ACTION_INGNO 0

#define sigset_check(sigset, num) (sigset & (1 << num))
#define sigset_add(sigset, num) (sigset |= (1 << num))
#define sigset_del(sigset, num) (sigset &= ~(1 << num))
#define sigset_copy(des, src) (des = src)

typedef unsigned long sigset_t;

struct sigaction
{
  void (*sa_handler)(int);
  sigset_t sa_mask;
  int sa_flags;
  void (*sa_restorer)();
};

struct sig_info
{
  sigset_t mask;
  sigset_t pending;
  struct sigaction actions[NSIG + 1];
};

void sig_check_signal();
void sig_send(struct task * task, int signum);
void sig_init();
int sig_task_init(struct task * task);
int sig_task_fork(struct task * des, struct task * src);
int sig_task_exec(struct task * task);
int sig_task_exit(struct task * task);
int sig_is_pending(struct task * task);
#endif
