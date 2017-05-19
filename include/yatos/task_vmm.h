/*
 *  Task virtutal memory management.
 *
 *  Copyright (C) 2017 ese@ccnt.zju
 *
 *  ---------------------------------------------------
 *  Started at 2017/4/13 by Ray
 *
 *  ---------------------------------------------------
 *
 *  This file is subject to the terms and conditions of the GNU General Public
 *  License.
 */

#ifndef __YATOS_TASK_VMM_H
#define __YATOS_TASK_VMM_H

#include <arch/system.h>
#include <yatos/list.h>
#include <yatos/mm.h>
#include <arch/mmu.h>

#define task_get_vmm_info(vmm_info) (vmm_info->count++)
#define task_put_vmm_info(vmm_info) \
  do{\
    vmm_info->count--;\
    if(!vmm_info->count)\
      slab_free_obj(vmm_info);\
  }while (0)

#define task_free_area(area) slab_free_obj(area)
#define task_vmm_switch_to(pre, next) \
  mmu_set_page_table(vaddr_to_paddr(next->mm_table_vaddr))

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
//just get a task_vmm_area
struct task_vmm_area * task_new_pure_area();
//alloc a area from vmm space
struct task_vmm_area * task_alloc_area(struct task_vmm_info * mm_info, int len);
int task_insert_area(struct task_vmm_info * vmm_info, struct task_vmm_area * area);
struct task_vmm_area * task_vmm_search_area(struct task_vmm_info * mm_info, unsigned long start_addr);
struct task_vmm_info * task_vmm_clone_info(struct task_vmm_info * from);
int task_copy_from_user(void * des, const void * src, unsigned long count);
int task_copy_to_user(void * des, const void * src, unsigned long count);
int task_copy_str_from_user(void * des, const char * str, unsigned long max_len);
int task_copy_pts_from_user(void * des, const char ** p, unsigned long max_len);
void task_vmm_clear(struct task_vmm_info *mm_info);

#endif /* __YATOS_TASK_VMM_H */
