#ifndef __YATOS_TASK_VMM_H
#define __YATOS_TASK_VMM_H

/*************************************************
 *   Author: Ray Huang
 *   Date  : 2017/4/13
 *   Email : rayhuang@126.com
 *   Desc  : task vmm manager
 ************************************************/
#include <arch/system.h>
#include <yatos/list.h>

struct task_vmm_area
{
  unsigned long start_addr;
  unsigned long len;
  unsigned long flag; //write ? executeable ?
  void *private;
  struct list_head list_entry;
  struct task_vmm_info * mm_info;
  void (*do_no_page)(struct task_vmm_area * area, unsigned long addr, char * new_page);
  void (*close)(struct task_vmm_area *area);
};

struct task_vmm_info
{
  unsigned long count;
  unsigned long mm_table_vaddr;
  struct task_vmm_area * stack;
  struct task_vmm_area * heap;
  struct list_head vmm_area_list;
};



void task_vmm_init();

struct task_vmm_info  * task_new_vmm_info();
void task_get_vmm_info(struct task_vmm_info * vmm_info);
void task_put_vmm_info(struct task_vmm_info * vmm_info);

//just get a task_vmm_area
struct task_vmm_area * task_new_pure_area();
void task_free_area(struct task_vmm_area *area);

//alloc a area from vmm space
struct task_vmm_area * task_alloc_area(struct task_vmm_info * mm_info, int len);

int task_insert_area(struct task_vmm_info * vmm_info, struct task_vmm_area * area);

struct task_vmm_area * task_vmm_search_area(struct task_vmm_info * mm_info, unsigned long start_addr);

struct task_vmm_info * task_vmm_clone_info(struct task_vmm_info * from);

void task_vmm_switch_to(struct task_vmm_info * pre, struct task_vmm_info *next);

#endif
