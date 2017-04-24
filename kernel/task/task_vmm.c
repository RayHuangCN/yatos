/*************************************************
 *   Author: Ray Huang
 *   Date  : 2017/4/13
 *   Email : rayhuang@126.com
 *   Desc  : task vmm manager
 ************************************************/
#include <arch/system.h>
#include <yatos/pmm.h>
#include <yatos/mm.h>
#include <yatos/task_vmm.h>
#include <yatos/task.h>
#include <yatos/irq.h>
#include <arch/mmu.h>
#include <yatos/schedule.h>

static struct kcache * vmm_info_cache;
static struct kcache * vmm_area_cache;

static struct irq_action page_fault_action;



static void vmm_info_constr(void *arg)
{
  struct task_vmm_info * vmm = (struct task_vmm_info*)arg;
  vmm->count = 1;
  vmm->mm_table_vaddr = 0;
  INIT_LIST_HEAD(&(vmm->vmm_area_list));
}

static void vmm_info_distr(void *arg)
{
  struct task_vmm_info * vmm = (struct task_vmm_info*)arg;
  task_vmm_clear(vmm);
  mm_kfree(vmm->mm_table_vaddr);

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
  printk("segment_error\n");
}

static int  page_access_fault(unsigned long addr, uint32 ecode)
{
  struct task * cur_task = task_get_cur();
  uint32 pdt_e = get_pdt_entry(cur_task->mm_info->mm_table_vaddr, addr);
  if (!pdt_e){
    printk("get empty pdt in page access fault\n");
    task_segment_error();
    return 1;
  }

  uint32 pet_e = get_pet_entry(paddr_to_vaddr(get_pet_addr(pdt_e)), addr);
  if (!pet_e){
    printk("get empty pet in page access fault\n");
    task_segment_error();
    return 1;
  }

  unsigned long page_paddr = get_pet_addr(pet_e);
  struct page * page = pmm_paddr_to_page(page_paddr);

  //if page->private is 0, this page is readonly
  //that is, this fault is a real access fault, we should kill task
  if (!page->private){
    task_segment_error();
    return 1;
  }
  else{
    //now we should do copy on write
    if (page->count == 1){ //we are the only one user of this page, so, it's not nessary to copy
      set_writable(pet_e);
      set_pet_entry(paddr_to_vaddr(get_pet_addr(pdt_e)), addr, pet_e);
      return 0;
    }
    pmm_put_one(page);
    unsigned long new_page = (unsigned long)mm_kmalloc(PAGE_SIZE);
    if (!new_page){
      task_segment_error();
      return 1;
    }
    struct page * pa = vaddr_to_page(new_page);
    pa->private = (void *)1;

    memcpy((void *)new_page, (void*)paddr_to_vaddr(page_paddr), PAGE_SIZE);
    //remap
    if (mmu_map(cur_task->mm_info->mm_table_vaddr, addr, vaddr_to_paddr(new_page), 1)){
      task_segment_error();
      return 1;
    }
  }
  return 0;
}

static int  page_fault_no_page(unsigned long fault_addr)
{
  struct task * cur_task = task_get_cur();
  struct task_vmm_info * mm_info = cur_task->mm_info;
  unsigned long fault_page_addr = PAGE_ALIGN(fault_addr);

  uint32 new_page_vaddr = (uint32)mm_kmalloc(PAGE_SIZE);
  if (!new_page_vaddr){
    task_segment_error();
    return 1;
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

  //for copy on wirte
  struct page * page = pmm_paddr_to_page(new_page_paddr);
  page->private = (void *)writeable;

  //now we setup mapping
  if (mmu_map(mm_info->mm_table_vaddr, fault_addr, new_page_paddr, writeable)){
    task_segment_error();
    return 1;
  }
  return 0;
}

static int task_vmm_do_page_fault(unsigned long fault_addr, uint32 ecode)
{
  if (fault_addr >= KERNEL_VMM_START){
    task_segment_error();
    return 1;
  }
  else if ((ecode & 1))
    return page_access_fault(fault_addr, ecode);
  else
    return page_fault_no_page(fault_addr);
}

static void task_vmm_page_fault(void *private, struct pt_regs * irq_context)
{
  uint32 ecode = irq_context->erro_code;
  unsigned long fault_addr = mmu_page_fault_addr();

  task_vmm_do_page_fault(fault_addr, ecode);
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

struct task_vmm_info * task_vmm_clone_info(struct task_vmm_info* from)
{
  struct task_vmm_info * ret = task_new_vmm_info();
  if (!ret)
    return NULL;
  //clone all vmm_area
  struct list_head *cur;
  struct task_vmm_area * cur_area;
  struct task_vmm_area * new_area;
  list_for_each(cur, &(from->vmm_area_list)){
    cur_area = container_of(cur, struct task_vmm_area, list_entry);
    new_area = task_new_pure_area();
    if (!new_area)
      goto new_area_error;
    memcpy(new_area, cur_area, sizeof(*cur_area));
    new_area->mm_info = ret;
    list_add_tail(&(new_area->list_entry), &(ret->vmm_area_list));

    if (cur_area == from->heap)
      ret->heap = new_area;
    if (cur_area == from->stack)
      ret->stack = new_area;
  }

  //get new pdt table
  ret->mm_table_vaddr = (unsigned long)mm_kmalloc(PAGE_SIZE);
  if (!ret->mm_table_vaddr)
    goto pdt_table_error;
  memset(ret->mm_table_vaddr, 0, PAGE_SIZE);
  //clone all pet table and setup copy on write
  uint32 * src_pdt = (uint32 *)from->mm_table_vaddr;
  uint32 * des_pdt = (uint32 *)ret->mm_table_vaddr;
  int i, j;
  //1. copy all kernel space pdt
  for (i = USER_SPACE_PDT_MAX_NUM; i < PDT_MAX_NUM; i++)
    des_pdt[i] = src_pdt[i];
  //2. clone user space and set up copy on write
  for (i = 0; i < USER_SPACE_PDT_MAX_NUM; i++){
    uint32 pdt_e = src_pdt[i];
    if (!pdt_e)
      continue;
    uint32 new_page_vaddr = (uint32)mm_kmalloc(PAGE_SIZE);
    if (!new_page_vaddr)
      goto pdt_table_error;

    uint32 new_page_paddr = vaddr_to_paddr(new_page_vaddr);
    des_pdt[i] = make_pdt(new_page_paddr, 1);

    //set up copy on write
    uint32 * des_pet = (uint32 *)new_page_vaddr;
    uint32 * src_pet = (uint32 *)paddr_to_vaddr(get_pet_addr(src_pdt[i]));
    memcpy(des_pet, src_pet, PAGE_SIZE);
    for (j = 0; j < PET_MAX_NUM; j++){
      if (!des_pet[j])
        continue;
      clr_writable(des_pet[j]);
      clr_writable(src_pet[j]); //src also need be readonly
      struct page* page = pmm_paddr_to_page(get_page_addr(des_pet[j]));
      pmm_get_one(page);
    }
  }

  return ret;

 pdt_table_error:
 new_area_error:
  list_for_each(cur, &(ret->vmm_area_list)){
    cur_area = container_of(cur, struct task_vmm_area, list_entry);
    task_free_area(cur_area);
  }
  task_put_vmm_info(ret);
  return NULL;
}


void task_vmm_switch_to(struct task_vmm_info* pre,struct task_vmm_info* next)
{
  mmu_set_page_table(vaddr_to_paddr(next->mm_table_vaddr));
}

static int task_check_user_space(unsigned long addr, unsigned long count, unsigned long rw)
{
  unsigned long page_vaddr = PAGE_ALIGN(addr);
  unsigned long pet_table_vaddr;
  uint32 pet_e, pdt_e;
  struct task * task = task_get_cur();

  while (page_vaddr + PAGE_SIZE - addr <= count){
    pdt_e = get_pdt_entry(task->mm_info->mm_table_vaddr, page_vaddr);
    if (!pdt_present(pdt_e) && task_vmm_do_page_fault(page_vaddr, 4))
      return 1;
    pdt_e = get_pdt_entry(task->mm_info->mm_table_vaddr, page_vaddr);

    pet_table_vaddr = paddr_to_vaddr(get_pet_addr(pdt_e));
    pet_e = get_pet_entry(pet_table_vaddr, page_vaddr);
    if (!pet_present(pet_e) && task_vmm_do_page_fault(page_vaddr, 4))
      return 1;
    pet_e = get_pet_entry(pet_table_vaddr, page_vaddr);
    if (!pet_writable(pet_e) && rw && task_vmm_do_page_fault(page_vaddr, 7))
      return 1;

    page_vaddr += PAGE_SIZE;
  }
  return 0;
}
int task_copy_from_user(void* des,const void* src,unsigned long count)
{
  if(task_check_user_space((unsigned long)src, count, 0))
    return 1;
  memcpy(des, src, count);
  return 0;
}

int task_copy_to_user(void* des,const void* src,unsigned long count)
{
  if (task_check_user_space((unsigned long)des, count, 1))
    return 1;
  memcpy(des, src, count);
  return 0;
}

void task_vmm_clear(struct task_vmm_info* vmm)
{
  // clean all vmm_areas
  struct list_head * cur;
  struct task_vmm_area * area;

  list_for_each(cur, &(vmm->vmm_area_list)){
    area = container_of(cur, struct task_vmm_area, list_entry);
    slab_free_obj(area);
  }
  // clean mm table
  uint32 * pdt_table = (uint32 *)vmm->mm_table_vaddr;
  uint32 * pet_table;
  unsigned long page_paddr;
  struct page * page;
  int i, j;

  if (!pdt_table)
    return ;
  for (i = 0; i < USER_SPACE_PDT_MAX_NUM; i++){

    if (!pdt_table[i])
      continue;
    pet_table = (uint32 *)paddr_to_vaddr(get_pet_addr(pdt_table[i]));
    for (j = 0; j < PET_MAX_NUM; j++){
      if (!pet_table[j])
        continue;
      page_paddr = get_page_addr(pet_table[j]);
      page = pmm_paddr_to_page(page_paddr);
      pmm_put_one(page);
    }
    pdt_table[i] = 0;
    mm_kfree(pet_table);
  }
  mmu_flush();
}

int task_copy_str_from_user(void* des,const char* str,unsigned long max_len)
{
  unsigned long  page_vaddr = PAGE_ALIGN((unsigned long)str);
  const char * cur = str;
  char * target = (char *)des;
  while (1){
    if (task_check_user_space(page_vaddr, PAGE_SIZE, 0))
      return 1;
    while (max_len && cur < (const char *)(page_vaddr + PAGE_SIZE) && (*target++ = *cur++) != '\0')
      max_len--;
    if (!max_len || *(target - 1) == '\0'){
      *target = '\0';
      return 0;
    }
    page_vaddr += PAGE_SIZE;
  }
  return 1;
}

int task_copy_pts_from_user(void* des,const char** p,unsigned long max_len)
{
  unsigned long page_vaddr = PAGE_ALIGN((unsigned long)p);
  const char ** cur = p;
  char ** target = (char **)des;
  while (1){
    if (task_check_user_space(page_vaddr, PAGE_SIZE, 0))
      return 1;

    while (max_len && cur < (const char **)(page_vaddr + PAGE_SIZE - 3) && (*target++ = *cur++) != NULL)
      max_len --;
    if (!max_len || *(target - 1) == '\0'){
      *target = NULL;
      return 0;
    }
    page_vaddr += PAGE_SIZE;

  }
  return 1;
}
