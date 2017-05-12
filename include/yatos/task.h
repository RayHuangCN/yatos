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
#include <yatos/bitmap.h>
#include <yatos/task_vmm.h>
#include <arch/regs.h>

#define KERNEL_STACK_SIZE (PAGE_SIZE<<1)


#define SECTION_WRITE 1
#define SECTION_ALLOC 2
#define SECTION_EXEC 4
#define SECTION_NOBITS 8


#define MAX_OPEN_FD 64
#define MAX_PID_NUM 256
#define MAX_ARG_NUM 32
#define MAX_ARG_LEN 64

#define TASK_USER_STACK_START 0xc0000000
#define TASK_USER_STACK_LEN   0x40000000
#define TASK_USER_HEAP_START (0xc0000000 - 0x80000000)
#define TASK_USER_HEAP_DEAULT_LEN 0
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


struct task_wait_entry
{
  struct task * task;
  void *private;
  void (* wake_up)(struct task * task, void * private);

  struct list_head wait_list_entry;
  struct list_head task_we_entry;
};

struct task_wait_queue
{
  struct list_head entry_list;
};

struct sig_info;
struct task
{
  unsigned long cur_stack;//must be the first one
  //task manage
  int  state;
  int  exit_status;
  int  pid;
  struct list_head task_list_entry;
  struct list_head run_list_entry;
  struct list_head wait_e_list;
  struct task * parent;
  struct list_head childs;
  struct list_head zombie_childs;
  struct list_head child_list_entry;
  int waitpid_blocked; //block in waitpid ?

  //schedule
  unsigned long remain_click;
  unsigned long need_sched;

  //exec and mm
  struct exec_bin * bin;
  unsigned long kernel_stack;
  struct task_vmm_info * mm_info;

  //opened file
  struct fs_file * files[MAX_OPEN_FD];
  struct bitmap * fd_map;
  struct bitmap * close_on_exec;

  //fs
  struct fs_file * cur_dir;

  //signal
  struct sig_info *sig_info;

  //tty
  int tty_num;
};


void task_init();

void task_setup_init(const char * path);

struct exec_bin * task_new_exec_bin();
struct section * task_new_section();
void task_put_bin(struct exec_bin * bin);
void task_get_bin(struct exec_bin * bin);


struct task_wait_queue * task_new_wait_queue();
struct task_wait_entry * task_new_wait_entry();
void task_free_wait_queue(struct task_wait_queue *queue);
void task_free_wait_entry(struct task_wait_entry * entry);

void task_init_wait_queue(struct task_wait_queue * queue);

void task_wait_on(struct task_wait_entry * entry, struct task_wait_queue * queue);
void task_notify_one(struct task_wait_queue * queue);
void task_notify_all(struct task_wait_queue * queue);
void task_leave_all_wq(struct task * task);
void task_leave_from_wq(struct task_wait_entry * wait_entry);
void task_segment_fault(struct task * task);
void task_exit(int status);
void task_gener_wake_up(struct task * task, void * private);

struct pt_regs * task_get_pt_regs(struct task * task);

#endif
