/*************************************************
 *   Author: Ray Huang
 *   Date  : 2017/5/12
 *   Email : rayhuang@126.com
 *   Desc  : signal
 ************************************************/
#include <yatos/signal.h>
#include <yatos/task.h>
#include <yatos/schedule.h>
#include <yatos/sys_call.h>
#include <yatos/mm.h>
#include <yatos/errno.h>
#include <arch/regs.h>

static struct kcache * sig_info_cache;

/**int sigaction(int signum, const struct sigaction *act,
   struct sigaction *oldact); **/
static int sys_call_signal(struct pt_regs * regs)
{
  int signum = (int)sys_call_arg1(regs);
  const struct sigaction * action = (const struct sigaction*)sys_call_arg2(regs);
  struct sigaction * oldact = (struct sigaction *)sys_call_arg3(regs);
  struct sig_info * sig_info = task_get_cur()->sig_info;
  struct sigaction * cur_action;
  struct sigaction buf;

  if (signum <=0 || signum > NSIG)
    return -EINVAL;
  cur_action = sig_info->actions + signum;

  if (oldact && task_copy_to_user(oldact, cur_action, sizeof(*cur_action)))
    return -EFAULT;
  if (action && task_copy_from_user(&buf, action, sizeof(buf)))
    return -EFAULT;
  if(action)
    memcpy(cur_action, &buf, sizeof(*cur_action));

  return 0;
}

/* regs is safe ? **/
static int sig_regs_is_safe(struct pt_regs * regs)
{
  return 1;
}

/** return from signal function
    any error will kill task O_o
 **/
static int sys_call_sigret(struct pt_regs * regs)
{
  char * user_sp = (char *)pt_regs_user_stack(regs);
  sigset_t oldmask;
  //up to PAGE ALIGN
  user_sp = (char *)PAGE_ALIGN(user_sp + PAGE_SIZE - 1);
  //check regs
  if (!sig_regs_is_safe((struct pt_regs *)user_sp))
    task_exit(SIGKILL);
  //copy old regs
  user_sp -= sizeof(*regs);
  if (task_copy_from_user(regs, user_sp, sizeof(*regs))){
    DEBUG("copy regs fault in sigret!");
    task_exit(SIGKILL);
  }
  //copy old sigmask
  user_sp -= sizeof(oldmask);
  if (task_copy_from_user(&oldmask, user_sp, sizeof(oldmask))){
    DEBUG("copy mask fault in sigret!");
    task_exit(SIGKILL);
  }
  task_get_cur()->sig_info->mask = oldmask;
  return 0;
}

//int sigprocmask(int how, const sigset_t *set, sigset_t *oldset);
static int sys_call_sigprocmask(struct pt_regs * regs)
{
  int how = (int)sys_call_arg1(regs);
  sigset_t * set = (sigset_t *)sys_call_arg2(regs);
  sigset_t * oldset = (sigset_t*)sys_call_arg3(regs);
  struct sig_info * sig_info = task_get_cur()->sig_info;
  sigset_t buf;
  int i;

  if (!sig_info){
    DEBUG("NULL task sig_info");
    return -EINVAL;
  }
  if (oldset && task_copy_to_user(oldset, &sig_info->mask, sizeof(sigset_t)))
    return -EFAULT;
  if (!set)
    return 0;
  if (task_copy_from_user(&buf, set, sizeof(sigset_t)))
    return -EFAULT;

  switch(how){
  case SIG_BLOCK:
    for (i = 1; i <= NSIG; i++)
      if (sigset_check(buf, i))
        sigset_add(sig_info->mask, i);
    return 0;

  case SIG_UNBLOCK:
    for (i = 1; i <= NSIG; i++)
      if (sigset_check(buf, i))
        sigset_del(sig_info->mask, i);
    return 0;

  case SIG_SET:
    sigset_copy(sig_info->mask, buf);
    return 0;
  }
  return -EINVAL;
}


void sig_init()
{
  sig_info_cache = slab_create_cache(sizeof(struct sig_info), NULL, NULL, "sig_info cache");
  if (!sig_info_cache)
    go_die("can not init sig cache");

  sys_call_regist(SYS_CALL_SIGNAL, sys_call_signal);
  sys_call_regist(SYS_CALL_SIGRET, sys_call_sigret);
  sys_call_regist(SYS_CALL_SIGPROCMASK, sys_call_sigprocmask);
}

void sig_send(struct task * task, int signum)
{
  struct sigaction * action;
  if (signum <= 0 || signum > NSIG)
    return ;
  action = task->sig_info->actions + signum;
  //SIGKILL, SIGSTOP, SIGCONT can not be ignored
  if (signum != SIGKILL && signum != SIGSTOP && signum != SIGCONT
      && action->sa_handler == SIG_IGN)
    return ;
  task->sig_info->pending |= 1 << signum;
  //force unblock blocked task
  task_ready_to_run(task);
}

/*** default action of SIG_DFL signal **/
static int sig_del_actions[NSIG + 1] = {
  [SIGINT] = SIG_ACTION_EXIT,
  [SIGQUIT] = SIG_ACTION_EXIT,
  [SIGABRT] = SIG_ACTION_EXIT,
  [SIGSEGV] = SIG_ACTION_EXIT
};

static void sig_do_signal(int num)
{
  struct task * task = task_get_cur();
  struct pt_regs * regs = task_get_pt_regs(task);
  struct sig_info  * sig_info = task->sig_info;
  char * user_sp = (char *)pt_regs_user_stack(regs);
  char * ret_code_start;
  unsigned long ret_code_len;
  unsigned long retaddr;
  //PAGE ALIGN
  user_sp = (char*)PAGE_ALIGN(user_sp);

  //copy pt_regs to user stack
  user_sp -= sizeof(*regs);
  if (task_copy_to_user(user_sp, regs, sizeof(*regs))){
    DEBUG("can not setup regs in sig_do_signal!");
    return ;
  }
  //copy old sigmask to user stack
  user_sp -= sizeof(sig_info->mask);
  if (task_copy_to_user(user_sp, &sig_info->mask, sizeof(sig_info->mask))){
    DEBUG("can not setup old maks in sig_do_signal!");
    return ;
  }
  //copy return code to user stack
  extern void sig_ret_code_start();
  extern void sig_ret_code_end();
  ret_code_start = (char *)sig_ret_code_start;
  ret_code_len = (char *)sig_ret_code_end - ret_code_start;

  user_sp -= sizeof(ret_code_len);
  if (task_copy_to_user(user_sp, ret_code_start, ret_code_len)){
    DEBUG("can not setup ret code in sig_do_signal!");
    return ;
  }
  retaddr = (unsigned long)user_sp;
  //setup params for signal function
  user_sp -= sizeof(int);
  if (task_copy_to_user(user_sp, &num, sizeof(int))){
    DEBUG("can not setup signal params in sig_do_signal!");
    return ;
  }
  //setup return address for signal function
  user_sp -= sizeof(unsigned long);
  if (task_copy_to_user(user_sp, &retaddr, sizeof(unsigned long))){
    DEBUG("can not setup return address in sig_do_signal!");
    return ;
  }
  //for jmp to signal function
  sig_info->mask = sig_info->actions[num].sa_mask;
  pt_regs_user_stack(regs) = (regs_type)user_sp;
  pt_regs_ret_addr(regs) = (regs_type)sig_info->actions[num].sa_handler;
}

void sig_check_signal()
{
  /*
  struct sig_info * sig_info = task_get_cur()->sig_info;
  struct sigaction * action = NULL;
  int i;
  //SIGKILL, SIGSTOP is special
  if (sigset_check(sig_info->pending, SIGKILL))
    task_exit(SIGKILL);
  if (sigset_check(sig_info->pending, SIGSTOP)){
    task_block(task_get_cur());
    task_schedule();
    return ;
  }

  //find a pending signal
  for (i = 1; i <= NSIG; i++){
    if (sigset_check(sig_info->pending, i)
        &&
        !sigset_check(sig_info->mask, i)){

      action = sig_info->actions + i;
      sigset_del(sig_info->pending, i);
      if (action->sa_handler == SIG_IGN)
        continue;
      else if (action->sa_handler == SIG_DFL){
        if (sig_del_actions[i] == SIG_ACTION_EXIT)
          task_exit(i);
      }else{
        sig_do_signal(i);
        break;
      }
    }
    }*/
}

int sig_task_init(struct task * task)
{
  task->sig_info = slab_alloc_obj(sig_info_cache);
  if (!task->sig_info)
    return -1;
  //mask it self
  int i;
  for (i = 1; i <= NSIG; i++)
    sigset_add(task->sig_info->actions[i].sa_mask, i);
  return 0;
}

int sig_task_fork(struct task * des, struct task * src)
{
  sig_task_init(des);
  memcpy(des->sig_info, src->sig_info, sizeof(struct sig_info));
  return 0;
}

int sig_task_exec(struct task* task)
{
  memset(task->sig_info, 0, sizeof(struct sig_info));
  return 0;
}

int sig_task_exit(struct task* task)
{
  slab_free_obj(task->sig_info);
  return 0;
}

int sig_is_pending(struct task * task)
{
  return task->sig_info->pending;
}
