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

char init_stack_space[KERNAL_STACK_SIZE];
static struct task * task_current;

static struct task *init;

static struct kcache * task_cache;
static struct list_head task_list;
static struct bitmap * task_map;

static struct list_head ready_listA;
static struct list_head ready_listB;
static struct list_head * cur_ready_list;

static void task_constr(void *arg)
{
  struct task * task = (struct task*)arg;
  memset(task, 0, sizeof(*task));
  INIT_LIST_HEAD(&(task->childs));
}

static void task_distr(void *arg)
{
  struct task * task = (struct task*)arg;
  bitmap_free(task->fd_map);
}

void task_init()
{
  task_cache = slab_create_cache(sizeof(struct task), task_constr, task_distr, "task cache");
  if (!task_cache)
    go_die("can not init task_cache\n");

  INIT_LIST_HEAD(&task_list);
  task_map = bitmap_create(MAX_PID_NUM);
  bitmap_alloc(task_map); //give up 0
  cur_ready_list = &ready_listA;


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


static int task_init_bin_areas(struct task_vmm_info * vmm_info, struct exec_bin *bin)
{


}


static void task_free_bin(struct exec_bin * bin)
{
  struct list_head * cur;
  struct section * sec;
  list_for_each(cur, &(bin->section_list)){
    sec = container_of(cur, struct section, list_entry);
    mm_kfree(sec);
  }
  mm_free(bin);
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

  init->bin = elf_parse(file);
  if (!init->bin)
    goto parse_file_error;
  if (task_init_bin_areas(init->mm_info, init->bin))
    goto init_bin_areas_error;

  init->remain_click = MAX_TASK_RUN_CLICK;



 init_bin_areas_error:
  task_free_bin(init->bin);
 parse_file_error:
  task_put_vmm_info(init->mm_info);
 create_mm_info_error:
  bitmap_destory(init->fd_map);
 create_fd_map_error:
  slab_free_obj(init);
 task_alloc_error:
  fs_close(file);
}
