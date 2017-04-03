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

char init_stack_space[4096 * 2];

static void kernel_banch()
{
  printk("================================================\n\r");
  printk("                  YatOS 0.11                    \n\r");
  printk("              Author: Ray Huang                 \n\r");
  printk("                      2017/4/02                 \n\r");
  printk("================================================\n\r");
}

void kernel_start()
{
  tty_init();
  irq_init();
  timer_init();
  kernel_banch();
  irq_enable();
  while (1);

}
