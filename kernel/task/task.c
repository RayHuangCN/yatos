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
char init_stack_space[KERNAL_STACK_SIZE];
static struct task * task_current;

static struct task *init;

static struct kcache * task_cache;
static struct kcache * bin_cache;
static struct kcache * section_cache;

static struct list_head task_list;
static struct bitmap * task_map;

static struct list_head ready_listA;
static struct list_head ready_listB;
static struct list_head * run_list;
static struct list_head * time_up_list;

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

void task_init()
{
  task_cache = slab_create_cache(sizeof(struct task), task_constr, task_distr, "task cache");
  bin_cache = slab_create_cache(sizeof(struct exec_bin), exec_bin_constr, exec_bin_distr, "bin cache");
  section_cache = slab_create_cache(sizeof(struct section), NULL, NULL, "section cache");
  if (!task_cache || !bin_cache || !section_cache)
    go_die("can not init caches\n");

  task_vmm_init();

  INIT_LIST_HEAD(&task_list);
  task_map = bitmap_create(MAX_PID_NUM);
  bitmap_alloc(task_map); //give up 0
  run_list = &ready_listA;
  time_up_list = &ready_listB;

  INIT_LIST_HEAD(run_list);
  INIT_LIST_HEAD(time_up_list);


}


static void tasK_schedule_to(struct task * prev, struct task *next)
{

}

void task_schedule()
{

}

struct task * task_get_cur()
{
  return task_current;
}



static void task_do_no_page(struct task_vmm_area * vmm_area, unsigned long fault_addr)
{

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
  task_do_no_page(stack_area, stack_addr - PAGE_SIZE);
  return 0;
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

static void task_add_new_task(struct task * new)
{
  irq_disable();
  list_add(&(new->task_list_entry), &(task_list));
  list_add(&(new->run_list_entry), run_list);
  irq_enable();
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
  if (!init->fd_map)
    goto create_fd_map_error;

  init->kernel_stack = (unsigned long)(init_stack_space + KERNAL_STACK_SIZE);
  init->parent = NULL;
  init->pid = bitmap_alloc(task_map);
  init->mm_info = task_new_vmm_info();
  if (!init->mm_info)
    goto create_mm_info_error;

  init->mm_info->mm_table_paddr = INIT_PDT_TABLE_START;
  if (!init->mm_info->mm_table_paddr)
    goto mm_table_alloc_error;

  init->bin = elf_parse(file);
  if (!init->bin)
    goto parse_file_error;

 if (task_init_bin_areas(init->mm_info, init->bin)
      || task_init_stack(init->mm_info, TASK_USER_STACK_START, TASK_USER_STACK_LEN)
      || task_init_heap(init->mm_info, TASK_USER_HEAP_START))
    goto init_mm_error;

  init->state = TASK_STATE_RUN;
  init->remain_click = MAX_TASK_RUN_CLICK;
  task_add_new_task(init);
  task_current = init;

  //this function never return
  task_arch_launch(init->bin->entry_addr, init->kernel_stack);
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
