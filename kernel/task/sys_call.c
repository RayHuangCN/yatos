/*************************************************
 *   Author: Ray Huang
 *   Date  : 2017/4/20
 *   Email : rayhuang@126.com
 *   Desc  : syscall manager
 ************************************************/
#include <yatos/sys_call.h>
#include <yatos/irq.h>

static struct irq_action sys_call_action;
static sys_call_fun sys_call_table[SYS_CALL_MAX_NNUM];

static void sys_call_despatch(void * private, struct pt_regs * regs)
{
  uint32 irq_save = arch_irq_save();
  arch_irq_enable();
  unsigned long num = regs->eax;
  if (num >= SYS_CALL_MAX_NNUM ||
      !sys_call_table[num]){
    regs->eax = -1;
    return ;
  }
  regs->eax = sys_call_table[num](regs);
  arch_irq_recover(irq_save);
}

void sys_call_init()
{
  irq_action_init(&sys_call_action);
  sys_call_action.action = sys_call_despatch;
  irq_regist(IRQ_SYSCALL, &sys_call_action);
}

void sys_call_regist(int sys_call_num,sys_call_fun do_sys_call)
{
  if (sys_call_num >= SYS_CALL_MAX_NNUM)
    return ;
  sys_call_table[sys_call_num] = do_sys_call;
}

void sys_call_unregist(int sys_call_num)
{
  if (sys_call_num >= SYS_CALL_MAX_NNUM)
    return ;
  sys_call_table[sys_call_num] = NULL;
}
