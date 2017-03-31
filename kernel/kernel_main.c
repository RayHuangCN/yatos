/*************************************************
*   Author: Ray Huang
*   Date  : 2017/3/30
*   Email : rayhuang@126.com
*   Desc  : first c file from asm code
************************************************/
#include <yatos/printk.h>
#include <yatos/tty.h>
#include <yatos/irq.h>
char init_stack_space[4096 * 2];

void kernel_start()
{

  tty_reset_cursor();
  tty_clear();
  irq_init();


  printk("kernel start..\n\r");

  extern void irq_test();
  irq_test();

  printk("after intr\n\r");

  while (1);
}
