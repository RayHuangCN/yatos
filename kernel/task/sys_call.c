/*
 *  System call management
 *
 *  Copyright (C) 2017 ese@ccnt.zju
 *
 *  ---------------------------------------------------
 *  Started at 2017/4/20 by Ray
 *
 *  ---------------------------------------------------
 *
 *  This file is subject to the terms and conditions of the GNU General Public
 *  License.
 */

#include <yatos/sys_call.h>
#include <yatos/irq.h>
#include <yatos/errno.h>

static struct irq_action sys_call_action;
static sys_call_fun sys_call_table[SYS_CALL_MAX_NNUM];

/*
 * This is the irq handler of system call.
 * This function will despatch the system call to the function int sys_call_table according to
 * system call number.
 */
static void sys_call_despatch(void * private, struct pt_regs * regs)
{
  uint32 irq_save = arch_irq_save();
  arch_irq_enable();
  unsigned long num = sys_call_num(regs);

  if (num >= SYS_CALL_MAX_NNUM ||
      !sys_call_table[num]){
    sys_call_ret(regs) = -EINVAL;
    return ;
  }
  sys_call_ret(regs) = sys_call_table[num](regs);
  arch_irq_recover(irq_save);
}

/*
 * Initate system call management.
 */
void sys_call_init()
{
  irq_action_init(&sys_call_action);
  sys_call_action.action = sys_call_despatch;
  irq_regist(IRQ_SYSCALL, &sys_call_action);
}

/*
 * Regist a system call to sys_call_table.
 */
void sys_call_regist(int sys_call_num,sys_call_fun do_sys_call)
{
  if (sys_call_num >= SYS_CALL_MAX_NNUM)
    return ;
  sys_call_table[sys_call_num] = do_sys_call;
}

/*
 * Unregist a system call to sys_call_table.
 */
void sys_call_unregist(int sys_call_num)
{
  if (sys_call_num >= SYS_CALL_MAX_NNUM)
    return ;
  sys_call_table[sys_call_num] = NULL;
}
