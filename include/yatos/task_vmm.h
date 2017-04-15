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
  struct list_hread list_entry;

  void (* open)(struct task_vmm_area * vmm_area);
  void (* do_page_fault)(struct task_vmm_area* vmm_area, unsigned long fault_addr);
  void (* close)(struct task_vmm_area * vmm_area);

};

struct task_vmm_info
{
  unsigned long count;
  unsigned long mm_table_paddr;
  struct list_head vmm_area_list;
};



void task_vmm_init();

struct task_vmm_info  * task_new_vmm_info();
void task_get_vmm_info(struct task_vmm_info * vmm_info);
void task_put_vmm_info(struct task_vmm_info * vmm_info);

//just get a task_vmm_area
struct task_vmm_area * task_new_pure_area();

//alloc a area from vmm space
struct task_vmm_area * task_alloc_area(int len);

void task_free_pure_area(struct task_vmm_area * area);

int task_install_area(struct task_vmm_info * vmm_info, struct task_vmm_area * area);




#endif
