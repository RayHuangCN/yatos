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

static void kernel_banch()
{
  printk("================================================\n");
  printk("                  YatOS 0.11                    \n");
  printk("              Author: Ray Huang                 \n");
  printk("                      2017/4/02                 \n");
  printk("================================================\n");
}

uint16 buffer[512];

void kernel_start()
{

  tty_init();
  mm_init();
  irq_init();
  timer_init();
  kernel_banch();
  fs_init();
  irq_enable();

  struct fs_file *ret = fs_open("huanglei/ray/work/main.c", root_dir);

  char buffer[100];
  int n = fs_read(ret, buffer, 100);
  int i;
  for (i = 0 ; i < n; i++)
    putc(buffer[i]);


  ret->inode->inode_data->i_ctime = 1234;
  fs_write(ret, "huanglei", sizeof("huanglei"));
  fs_sync(ret);



  while (1);
}
