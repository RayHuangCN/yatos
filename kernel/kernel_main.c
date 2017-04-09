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
#include <arch/disk.h>
static void kernel_banch()
{
  printk("================================================\n\r");
  printk("                  YatOS 0.11                    \n\r");
  printk("              Author: Ray Huang                 \n\r");
  printk("                      2017/4/02                 \n\r");
  printk("================================================\n\r");
}

uint16 buffer[512];

void kernel_start()
{

  tty_init();
  mm_init();
  irq_init();
  timer_init();
  kernel_banch();
  irq_enable();


  printk("start\n\r");
  int i, j;
  for (i = 0 ; i < 512; i++)
    buffer[i] = i;

  disk_write(8192, 1, buffer);
  for (i = 0 ; i < 512; i++)
    buffer[i] = 0;
  disk_read(8192, 1, buffer);
  for (j = 0; j < 20; j++)
    printk("%04x  ", buffer[j]);


  while (1);
}
