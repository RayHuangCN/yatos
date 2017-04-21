/*************************************************
 *   Author: Ray Huang
 *   Date  : 2017/4/4
 *   Email : rayhuang@126.com
 *   Desc  : taks manager
 ************************************************/
#include <yatos/irq.h>
#include <yatos/task.h>
#include <yatos/elf.h>
#include <yatos/mm.h>
#include <yatos/bitmap.h>
#include <arch/task.h>
#include <arch/mmu.h>
#include <yatos/sys_call.h>
#include <yatos/schedule.h>

char init_stack_space[KERNEL_STACK_SIZE];
static struct task *init;
static struct kcache * task_cache;
static struct kcache * bin_cache;
static struct kcache * section_cache;
static struct bitmap * task_map;

static void task_constr(void *arg)
{
  struct task * task = (struct task*)arg;
  INIT_LIST_HEAD(&(task->childs));
}

static void task_distr(void *arg)
{
  struct task * task = (struct task*)arg;
  if (task->fd_map)
    bitmap_destory(task->fd_map);
  if (task->mm_info)
    task_put_vmm_info(task->mm_info);
  if (task->bin)
    task_free_bin(task->bin);
}
static void exec_bin_constr(void *arg)
{
  struct exec_bin * bin = (struct exec_bin*)arg;
  bin->count = 1;
  INIT_LIST_HEAD(&(bin->section_list));
}

static void exec_bin_distr(void *arg)
{
  struct exec_bin *bin = (struct exec_bin*)arg;
  struct list_head *cur;
  struct section * cur_se;
  list_for_each(cur, &(bin->section_list)){
    cur_se = container_of(cur, struct section, list_entry);
    slab_free_obj(cur_se);
  }
}

static void task_get_bin(struct exec_bin *bin)
{
  bin->count++;
}

static void task_put_bin(struct exec_bin *bin)
{
  bin->count--;
  if (!bin->count)
    slab_free_obj(bin);
}

static void task_do_no_page(struct task_vmm_area * vmm_area, unsigned long fault_addr, char * new_page)
{
  uint32 fault_page_addr = PAGE_ALIGN(fault_addr);
  uint32 page_offset, read_offset, read_len;
  uint32 left_max, right_min;
  if (vmm_area->start_addr < fault_page_addr){
    page_offset = 0;
    read_offset = fault_page_addr - vmm_area->start_addr;
    left_max = fault_page_addr;
  }else{
    page_offset = vmm_area->start_addr - fault_page_addr;
    read_offset = 0;
    left_max = vmm_area->start_addr;
  }

  if (vmm_area->start_addr + vmm_area->len > fault_page_addr + PAGE_SIZE)
    right_min = fault_page_addr + PAGE_SIZE;
  else
    right_min = vmm_area->start_addr + vmm_area->len;

  read_len = right_min - left_max;

  struct fs_file * file = task_get_cur()->bin->exec_file;
  struct section * sec = (struct section *)vmm_area->private;
  if (vmm_area->flag & SECTION_NOBITS)
    memset(new_page + page_offset,0 ,read_len);
  else{
    fs_seek(file, sec->file_offset + read_offset, SEEK_SET);
    fs_read(file, new_page + page_offset, read_len);
  }

}



static int task_init_bin_areas(struct task_vmm_info * vmm_info, struct exec_bin *bin)
{
  struct list_head * cur;
  struct section * cur_sec;
  struct task_vmm_area * cur_area;

  list_for_each(cur, &(bin->section_list)){
    cur_sec = container_of(cur, struct section, list_entry);
    cur_area = task_new_pure_area();
    if (!cur_area)
      return 0;
    cur_area->start_addr = cur_sec->start_vaddr;
    cur_area->len = cur_sec->len;
    cur_area->flag = cur_sec->flag;
    cur_area->private = cur_sec;
    cur_area->mm_info = vmm_info;
    cur_area->do_no_page = task_do_no_page;

    if (task_insert_area(vmm_info, cur_area)){
      printk("insert area error\n");
      return 1;
    }
  }
  return 0;
}

static void task_adopt(struct task * parent, struct task *child)
{
  child->parent = parent;
  list_add_tail(&(child->child_list_entry), &(parent->childs));
}

//fork new task
static int sys_call_fork(struct pt_regs * regs)
{
  struct task * new_task = slab_alloc_obj(task_cache);
  struct task * cur_task = task_get_cur();
  if (!new_task)
    return -1;
  memcpy(new_task, cur_task, sizeof(*new_task));

  new_task->pid = bitmap_alloc(task_map);
  INIT_LIST_HEAD(&(new_task->childs));

  //kernel stack should be new
  unsigned long stack = (unsigned long)mm_kmalloc(KERNEL_STACK_SIZE);
  if (!stack)
    goto alloc_stack_error;
  new_task->kernel_stack = stack + KERNEL_STACK_SIZE;
  memcpy((void *)stack, (void *)(cur_task->kernel_stack - KERNEL_STACK_SIZE), KERNEL_STACK_SIZE);

  new_task->remain_click = MAX_TASK_RUN_CLICK;

  //fds should be new
  new_task->fd_map = bitmap_clone(cur_task->fd_map);
  new_task->no_close_on_exec = bitmap_clone(cur_task->no_close_on_exec);
  if (!new_task->fd_map || new_task->no_close_on_exec)
    goto bitmap_clone_error;

  //mm_info should be new, and should use copy on write
  new_task->mm_info = task_vmm_clone_info(cur_task->mm_info);
  if (!new_task->mm_info)
    goto vmm_info_clone_error;

  //update all count of inference
  task_get_bin(cur_task->bin);
  int i;
  for (i = 0; i < MAX_OPEN_FD; i++)
    if (cur_task->files[i])
      fs_get_file(cur_task->files[i]);

  //setup task relationship
  task_adopt(cur_task, new_task);

  //add to manager list
  task_add_new_task(new_task);

  //make new_task scheduleable
  task_arch_init_run_context(new_task, 0);

  return new_task->pid;

  //error
 vmm_info_clone_error:
  bitmap_destory(new_task->fd_map);
 bitmap_clone_error:
  mm_kfree(stack);
 alloc_stack_error:
  slab_free_obj(new_task);
  return -1;
}

//return cur stack addr
static unsigned long task_init_args(struct task_vmm_area *stack, char * args[])
{
  return stack->start_addr + stack->len - 12;
}


static int task_do_execve(const char *path, char * argv[], char * envp[])
{
  struct task * task = task_get_cur();
  task_put_bin(task->bin);
  int i;
  for (i = 0 ; i < MAX_OPEN_FD; i++)
    if (task->files[i] && !bitmap_check(task->no_close_on_exec, i)){
      fs_close(task->files[i]);
      bitmap_free(task->fd_map, i);
    }

  //rebuild mm_info
  task_vmm_clear(task->mm_info);
  task->bin = fs_open(path, root_dir);




  return 1;
}

static int sys_call_execve(struct pt_regs * regs)
{
  const char * path_name = (const char *)regs->ebx;
  char ** argv = (char **)regs->ecx;
  char ** envp = (char **)regs->edx;
  return task_do_execve(path_name, argv, envp);
}

static int sys_call_exit(struct pt_regs * regs)
{
  int status = regs->ebx;
  printk("sys_call_exit\n");
  return 0;
}

static int sys_call_test3(struct pt_regs * regs)
{
  printk("sys_call_3\n");
  return 0;
}

static int sys_call_test4(struct pt_regs * regs)
{
  printk("sys_call_4\n");
  return 0;
}



void task_init()
{
  task_arch_init();
  task_cache = slab_create_cache(sizeof(struct task), task_constr, task_distr, "task cache");
  bin_cache = slab_create_cache(sizeof(struct exec_bin), exec_bin_constr, exec_bin_distr, "bin cache");
  section_cache = slab_create_cache(sizeof(struct section), NULL, NULL, "section cache");
  if (!task_cache || !bin_cache || !section_cache)
    go_die("can not init caches\n");

  task_vmm_init();
  task_schedule_init();

  task_map = bitmap_create(MAX_PID_NUM);
  bitmap_alloc(task_map); //give up

  sys_call_init();
  //regist sys_call
  sys_call_regist(SYS_CALL_FORK, sys_call_fork);
  sys_call_regist(SYS_CALL_EXIT, sys_call_exit);
  sys_call_regist(3, sys_call_test3);
  sys_call_regist(4, sys_call_test4);
}







static int task_init_stack(struct task_vmm_info * vmm_info, unsigned long stack_addr, unsigned long len)
{
  //set up area
  struct task_vmm_area * stack_area = task_new_pure_area();
  stack_area->do_no_page = task_do_no_page;
  stack_area->flag = SECTION_WRITE | SECTION_NOBITS;
  stack_area->mm_info = vmm_info;
  stack_area->start_addr = stack_addr - len;
  stack_area->len = len;
  if (task_insert_area(vmm_info, stack_area)){
    task_free_area(stack_area);
    return 1;
  }
  //we need map 1 page to init exec args
  uint32 new_page_vaddr = (uint32)mm_kmalloc(PAGE_SIZE);

  if (!new_page_vaddr)
    return 1;

  vmm_info->stack = stack_area;
  uint32 new_page_paddr = vaddr_to_paddr(new_page_vaddr);
  task_do_no_page(stack_area, stack_addr - PAGE_SIZE, (char *)new_page_vaddr);
  vaddr_to_page(new_page_vaddr)->private = 1;

  return mmu_map(vmm_info->mm_table_vaddr, stack_addr - PAGE_SIZE, new_page_paddr, 1);
}

static int task_init_heap(struct task_vmm_info * vmm_info, unsigned long heap_addr)
{
  //set up heap
  struct task_vmm_area * heap_area = task_new_pure_area();
  heap_area->do_no_page = task_do_no_page;
  heap_area->flag = SECTION_WRITE | SECTION_NOBITS;
  heap_area->mm_info = vmm_info;
  heap_area->start_addr = heap_addr;
  heap_area->len = TASK_USER_HEAP_DEAULT_LEN;
  if (task_insert_area(vmm_info, heap_area)){
    task_free_area(heap_area);
    return 1;
  }
  vmm_info->heap = heap_area;
  return 0;
}



void task_free_bin(struct exec_bin * bin)
{
  struct list_head * cur;
  struct section * sec;
  list_for_each(cur, &(bin->section_list)){
    sec = container_of(cur, struct section, list_entry);
    mm_kfree(sec);
  }
  mm_kfree(bin);
}



void task_setup_init(const char* path)
{
  struct fs_file * file = fs_open(path, root_dir);
  if (!file){
    printk("can not open %s\n", path);
    return ;
  }

  init = slab_alloc_obj(task_cache);
  if (!init)
    goto task_alloc_error;

  init->fd_map = bitmap_create(MAX_OPEN_FD);
  init->no_close_on_exec = bitmap_create(MAX_OPEN_FD);
  if (!init->fd_map || !init->no_close_on_exec)
    goto create_fd_map_error;

  init->kernel_stack = (unsigned long)(init_stack_space + KERNEL_STACK_SIZE);
  init->parent = NULL;
  init->pid = bitmap_alloc(task_map);
  init->mm_info = task_new_vmm_info();
  if (!init->mm_info)
    goto create_mm_info_error;

  init->mm_info->mm_table_vaddr = INIT_PDT_TABLE_START;
  if (!init->mm_info->mm_table_vaddr)
    goto mm_table_alloc_error;

  init->bin = elf_parse(file);
  if (!init->bin)
    goto parse_file_error;

  if (task_init_bin_areas(init->mm_info, init->bin)
      || task_init_stack(init->mm_info, TASK_USER_STACK_START, TASK_USER_STACK_LEN)
      || task_init_heap(init->mm_info, TASK_USER_HEAP_START))
    goto init_mm_error;

  uint32 cur_stack = task_init_args(init->mm_info->stack, NULL);

  init->state = TASK_STATE_RUN;
  init->remain_click = MAX_TASK_RUN_CLICK;
  task_add_new_task(init);

  task_arch_befor_launch(init);
  //this function never return
  task_arch_launch(init->bin->entry_addr, cur_stack);
  /******** never back here ***********************/

 init_mm_error:
  task_free_bin(init->bin);

 parse_file_error:
  task_put_vmm_info(init->mm_info);

 mm_table_alloc_error:
  task_put_vmm_info(init->mm_info);

 create_mm_info_error:
  bitmap_destory(init->fd_map);

 create_fd_map_error:
  slab_free_obj(init);

 task_alloc_error:
  fs_close(file);
}

struct exec_bin * task_new_exec_bin()
{
  slab_alloc_obj(bin_cache);
}

struct section * task_new_section()
{
  slab_alloc_obj(section_cache);
}
