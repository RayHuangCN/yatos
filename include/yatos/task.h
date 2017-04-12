#ifndef __YATOS_TASK_H
#define __YATOS_TASK_H

/*************************************************
 *   Author: Ray Huang
 *   Date  : 2017/4/4
 *   Email : rayhuang@126.com
 *   Desc  : task manager interface
 ************************************************/
#include <yatos/elf.h>
#include <yatos/printk.h>
#include <yatos/list.h>
#include <yatos/mm.h>
#include <yatos/fs.h>

#define TASK_STATE_RUN 1
#define TASK_STATE_STOP 2
#define TASK_STATE_ZOMBIE 3

#define SECTION_WRITE 1
#define SECTION_ALLOC 2
#define SECTION_EXEC 4
#define SECTION_NOBITS 8


#define MAX_OPEN_FD 64

struct section
{
  uint32 start_vaddr;
  uint32 len;
  uint32 flag;
  uint32 file_offset;
  struct list_head list_entry;
};


struct exec_bin
{
  uint32 entry_addr;
  struct fs_file *exec_file;
  unsigned long count;
  struct list_head section_list;
};


struct task
{
  //task manage
  unsigned long state;
  unsigned long pid;
  unsigned long ppid;
  struct list_head task_list_entry;
  struct task * parent;
  struct list_head childs;
  struct list_head child_list_entry;

  //schedule
  unsigned long remain_click;

  //exec and mm
  struct ext_bin * bin;


  //opened file
  struct file * files[MAX_OPEN_FD];
  uint32 fd_map[MAX_OPEN_FD / 32];

  //signal
};





#endif
