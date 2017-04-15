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

  struct fs_file *ret = fs_open("/sbin/init", root_dir);
  if (!ret){
    printk("can not open init\n");
    while (1);
  }
  struct exec_bin * bin = elf_parse(ret);


  struct list_head *cur;
  struct section * cur_s;
  printk("entry = %x\n", bin->entry_addr);
  list_for_each(cur, &(bin->section_list)){

    cur_s = container_of(cur, struct section, list_entry);
    printk("======================\n");
    printk("addr = %x\n", cur_s->start_vaddr);
    printk("len = %x\n", cur_s->len);
    printk("file_offset = %x\n", cur_s->file_offset);
  }


  while (1);
}
