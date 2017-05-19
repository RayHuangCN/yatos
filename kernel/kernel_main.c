/*
 *  This is the first c file from Start.asm
 *
 *  Copyright (C) 2017 ese@ccnt.zju
 *
 *  ---------------------------------------------------
 *  Started at 2017/3/30 by Ray
 *
 *  ---------------------------------------------------
 *
 *  This file is subject to the terms and conditions of the GNU General Public
 *  License.
 */

#include <yatos/printk.h>
#include <yatos/tty.h>
#include <yatos/irq.h>
#include <yatos/list.h>
#include <yatos/timer.h>
#include <yatos/mm.h>
#include <yatos/fs.h>
#include <yatos/task.h>
#include <arch/asm.h>
#include <yatos/ipc.h>


 /*
  * Jmp from start.asm, this is the top level function of yatos
  * the first task named "init" will be stared after all initiates
  */
void kernel_start()
{
  tty_reset_cursor();
  tty_clear();

  mm_init();
  irq_init();
  timer_init();
  task_init();
  fs_init();
  tty_init();
  ipc_init();

  task_setup_init("/sbin/init");
  /* never back here */
  printk("can not setup init!");
  while (1);
}
