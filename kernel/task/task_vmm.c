/*************************************************
 *   Author: Ray Huang
 *   Date  : 2017/4/13
 *   Email : rayhuang@126.com
 *   Desc  : task vmm manager
 ************************************************/
#include <arch/system.h>
#include <yatos/mm.h>
#include <yatos/task_vmm.h>
#include <yatos/task.h>
#include <yatos/irq.h>
#include <arch/mmu.h>
static struct kcache * vmm_info_cache;
static struct kcache * vmm_area_cache;

static struct irq_action page_fault_action;



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


static void task_segment_error()
{

}

static void page_access_fault(unsigned long addr, uint32 ecode)
{

}

static void page_fault_no_page(unsigned long fault_addr)
{
  struct task * cur_task = task_get_cur();
  struct task_vmm_info * mm_info = cur_task->mm_info;
  unsigned long fault_page_addr = PAGE_ALIGN(fault_addr);

  uint32 new_page_vaddr = (uint32)mm_kmalloc(PAGE_SIZE);
  if (!new_page_vaddr){
    task_segment_error();
    return ;
  }

  uint32 new_page_paddr = vaddr_to_paddr(new_page_vaddr);
  uint32 writeable = 0;


  //now we should init the content of new page
  //content come from do_no_page function  of vmm_areas
  //if any vmm_area is writable, this page should be wirteable.

  struct list_head *cur;
  struct task_vmm_area *area;
  list_for_each(cur, &(mm_info->vmm_area_list)){
    area = container_of(cur, struct task_vmm_area, list_entry);
    if ((area->start_addr >= fault_page_addr
         && area->start_addr < fault_page_addr + PAGE_SIZE)
        ||
        (area->start_addr + area->len >= fault_page_addr
         && area->start_addr + area->len <= fault_page_addr + PAGE_SIZE)
        ||
        (area->start_addr < fault_page_addr
         && area->start_addr + area->len >= fault_page_addr + PAGE_SIZE)){

      if (area->flag & SECTION_WRITE)
        writeable = 1;
      if (area->do_no_page)
        area->do_no_page(area,fault_addr, (char *)new_page_vaddr);
    }else if (area->start_addr >= fault_page_addr + PAGE_SIZE)
      break;
  }

  //now we setup mapping
  if (mmu_map(mm_info->mm_table_paddr, fault_addr, new_page_paddr, writeable))
    task_segment_error();
}

static void task_vmm_page_fault(void *private, struct pt_regs * irq_context)
{
  uint32 ecode = irq_context->erro_code;
  unsigned long fault_addr = mmu_page_fault_addr();

  if (fault_addr >= KERNEL_VMM_START)
    task_segment_error();
  else if (ecode & 1)
    page_access_fault(fault_addr, ecode);
  else
    page_fault_no_page(fault_addr);
}


void task_vmm_init()
{
  vmm_info_cache = slab_create_cache(sizeof(struct task_vmm_info), vmm_info_constr, vmm_info_distr, "vmm_info cache");
  vmm_area_cache = slab_create_cache(sizeof(struct task_vmm_area), vmm_area_constr, vmm_area_distr,"vmm_area cache");

  if (!vmm_info_cache || !vmm_area_cache)
    go_die("can not task cache error\n");

  //init page fault
  irq_action_init(&page_fault_action);
  page_fault_action.action = task_vmm_page_fault;
  irq_regist(IRQ_PAGE_FAULT, &page_fault_action);
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
