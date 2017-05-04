/*************************************************
*   Author: Ray Huang
*   Date  : 2017/3/30
*   Email : rayhuang@126.com
*   Desc  : first c file from asm code
************************************************/
#include <yatos/printk.h>
#include <yatos/tty.h>
#include <yatos/irq.h>
#include <yatos/list.h>
#include <yatos/timer.h>
#include <yatos/mm.h>
#include <yatos/fs.h>
#include <yatos/task.h>
#include <arch/asm.h>
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

  task_setup_init("/sbin/init");
  /* never back here */
  go_die("the end of the world\n");
  while (1);
}
