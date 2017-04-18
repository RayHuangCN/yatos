/*************************************************
 *   Author: Ray Huang
 *   Date  : 2017/4/13
 *   Email : rayhuang@126.com
 *   Desc  : task vmm manager
 ************************************************/
#include <arch/system.h>
#include <yatos/mm.h>
#include <yatos/task_vmm.h>
static struct kcache * vmm_info_cache;
static struct kcache * vmm_area_cache;



static void vmm_info_constr(void *arg)
{
  struct task_vmm_info * vmm = (struct task_vmm_info*)arg;
  vmm->count = 1;
  vmm->mm_table_paddr = 0;
  INIT_LIST_HEAD(&(vmm->vmm_area_list));
}

static void vmm_info_distr(void *arg)
{
  struct task_vmm_info * vmm = (struct task_vmm_info*)arg;
  struct list_head * cur;
  struct task_vmm_area * area;

  list_for_each(cur, &(vmm->vmm_area_list)){
    area = container_of(cur, struct task_vmm_area, list_entry);
    slab_free_obj(area);
  }
}

static void vmm_area_constr(void *arg)
{
  struct task_vmm_area * area = (struct task_vmm_area *)arg;
  memset(area, 0, sizeof(*area));
}

static void vmm_area_distr(void *arg)
{
  struct task_vmm_area * area = (struct task_vmm_area *)arg;
  if (area->close)
    area->close(area);
}


void task_vmm_init()
{
  vmm_info_cache = slab_create_cache(sizeof(struct task_vmm_info), vmm_info_constr, vmm_info_distr, "vmm_info cache");
  vmm_area_cache = slab_create_cache(sizeof(struct task_vmm_area), vmm_area_constr, vmm_area_distr,"vmm_area cache");

  if (!vmm_info_cache || !vmm_area_cache)
    go_die("can not task cache error\n");
}

struct task_vmm_info * task_new_vmm_info()
{
  return slab_alloc_obj(vmm_info_cache);
}

void task_get_vmm_info(struct task_vmm_info* vmm_info)
{
  vmm_info->count++;
}

void task_put_vmm_info(struct task_vmm_info* vmm_info)
{
  vmm_info->count--;

  if (!vmm_info->count)
    slab_free_obj(vmm_info);
}

struct task_vmm_area * task_get_pure_area()
{
  return slab_alloc_obj(vmm_area_cache);
}

int task_insert_area(struct task_vmm_info* vmm_info,struct task_vmm_area* area)
{
  struct task_vmm_area * pre;

  if (list_empty(&(vmm_info->vmm_area_list))){
      list_add_tail(&(area->list_entry), &(vmm_info->vmm_area_list));
      return 0;
  }

  pre = task_vmm_search_area(vmm_info, area->start_addr);
  if (!pre)
    list_add(&(area->list_entry), &(vmm_info->vmm_area_list));
  else{
    if (pre->start_addr + pre->len > area->start_addr){
      printk("vmm area overlap!\n");
      return 1;
    }
    if (list_is_last(&(area->list_entry), &(vmm_info->vmm_area_list)))
      list_add_tail(&(area->list_entry), &(vmm_info->vmm_area_list));
    else
      list_add(&(area->list_entry), &(pre->list_entry));
  }
  return 0;
}

struct task_vmm_area * task_vmm_search_area(struct task_vmm_info * mm_info, unsigned long start_addr)
{
  struct list_head *cur;
  struct task_vmm_area * cur_area, *next_area;
  list_for_each(cur, &(mm_info->vmm_area_list)){
    cur_area = container_of(cur, struct task_vmm_area, list_entry);
    if (cur_area->start_addr <= start_addr){
      if (list_is_last(cur, &(mm_info->vmm_area_list)))
        return cur_area;
      next_area = container_of(cur->next, struct task_vmm_area, list_entry);
      if (next_area->start_addr > start_addr)
        return cur_area;
    }
  }
  return NULL;
}

void task_free_area(struct task_vmm_area* area)
{
  slab_free_obj(area);
}

struct task_vmm_area * task_new_pure_area()
{
  return slab_alloc_obj(vmm_area_cache);
}
