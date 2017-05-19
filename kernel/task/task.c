/*
 *  Task life cycle management
 *
 *  Copyright (C) 2017 ese@ccnt.zju
 *
 *  ---------------------------------------------------
 *  Started at 2017/4/4 by Ray
 *
 *  ---------------------------------------------------
 *
 *  This file is subject to the terms and conditions of the GNU General Public
 *  License.
 */

#include <yatos/irq.h>
#include <yatos/task.h>
#include <yatos/elf.h>
#include <yatos/mm.h>
#include <yatos/bitmap.h>
#include <arch/task.h>
#include <arch/mmu.h>
#include <yatos/sys_call.h>
#include <yatos/schedule.h>
#include <yatos/fs.h>
#include <yatos/errno.h>
#include <yatos/signal.h>

char init_stack_space[KERNEL_STACK_SIZE];
static struct task *init;
static struct kcache * task_cache;
static struct kcache * bin_cache;
static struct kcache * section_cache;
static struct kcache * wait_entry_cache;
static struct kcache * wait_queue_cache;
static struct bitmap * task_map;

/*
 * Constructor of "struct task".
 * This function will be called once we alloc a new task.
 */
static void task_constr(void *arg)
{
  struct task * task = (struct task*)arg;
  INIT_LIST_HEAD(&(task->childs));
  INIT_LIST_HEAD(&(task->wait_e_list));
  INIT_LIST_HEAD(&(task->zombie_childs));
  task->tty_num = -1;
}

/*
 * Constructor of "struct exec_bin".
 * This function will be called once we alloc a exec_bin.
 */
static void exec_bin_constr(void *arg)
{
  struct exec_bin * bin = (struct exec_bin*)arg;
  bin->count = 1;
  INIT_LIST_HEAD(&(bin->section_list));
}

/*
 * Destructor of "exec_bin".
 * This function will be called once we destory a exec_bin.
 */
static void exec_bin_distr(void *arg)
{
  struct exec_bin *bin = (struct exec_bin*)arg;
  struct list_head *cur;
  struct list_head * next;
  struct section * cur_se;
  list_for_each_safe(cur, next, &(bin->section_list)){
    cur_se = container_of(cur, struct section, list_entry);
    slab_free_obj(cur_se);
  }
}

/*
 * Fill a area of a page according to "vmm_area".
 * This function may read data from disk or fill all zero when the vmm_aera with the flag of
 * the SECTION_NOBITS.
 * This function will be called in page fault trap handler.
 *
 * Note: This is not the handler of page fault!.
 */
static void task_do_no_page(struct task_vmm_area * vmm_area, unsigned long fault_addr, char * new_page)
{
  uint32 fault_page_addr = PAGE_ALIGN(fault_addr);
  uint32 page_offset, read_offset, read_len;
  uint32 left_max, right_min;
  struct fs_file * file;
  struct section * sec;

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

  file = task_get_cur()->bin->exec_file;
  sec = (struct section *)vmm_area->private;
  if (vmm_area->flag & SECTION_NOBITS)
    memset(new_page + page_offset,0 ,read_len);
  else{
    fs_seek(file, sec->file_offset + read_offset, SEEK_SET);
    fs_read(file, new_page + page_offset, read_len);
  }
}

/*
 * Setup the vmm area of user stack and add to "vmm_info".
 * Return 0 if successful or return -1 if any error.
 */
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
    return -1;
  }
  vmm_info->stack = stack_area;
  return 0;
}

/*
 * Setup the vmm aera of user heap and add to "vmm_info".
 * Return 0 if successful or return -1 if any error.
 */
static int task_init_heap(struct task_vmm_info * vmm_info, unsigned long heap_addr, unsigned long len)
{
  //set up heap
  struct task_vmm_area * heap_area = task_new_pure_area();
  heap_area->do_no_page = task_do_no_page;
  heap_area->flag = SECTION_WRITE | SECTION_NOBITS;
  heap_area->mm_info = vmm_info;
  heap_area->start_addr = heap_addr;
  heap_area->len = len;
  if (task_insert_area(vmm_info, heap_area)){
    task_free_area(heap_area);
    return -1;
  }
  vmm_info->heap = heap_area;
  return 0;
}

/*
 * Setup all normal vmm aera according to exec_bin which is parse from elf file.
 * Return 0 if successful or return -1 if any error.
 */
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
      DEBUG("insert area error\n");
      return -1;
    }
  }
  return 0;
}

/*
 * To make "parent" to be the parent task of "child".
 */
static void task_adopt(struct task * parent, struct task *child)
{
  child->parent = parent;
  list_add_tail(&(child->child_list_entry), &(parent->childs));
}

/*
 * System call of fork.
 * Clone a new task from current task.
 * Return 0 if successful or return error code if any error.
 */
static int sys_call_fork(struct pt_regs * regs)
{
  struct task * new_task = slab_alloc_obj(task_cache);
  struct task * cur_task = task_get_cur();
  int ret = -1;
  int i;
  unsigned long stack;

  if (!new_task){
    DEBUG("sys_call_fork can not alloc new_task\n");
    return -ENOMEM;
  }
  memcpy(new_task, cur_task, sizeof(*new_task));
  new_task->pid = bitmap_alloc(task_map);
  INIT_LIST_HEAD(&(new_task->childs));
  INIT_LIST_HEAD(&(new_task->zombie_childs));
  INIT_LIST_HEAD(&(new_task->wait_e_list));

  //kernel stack should be new
  stack = (unsigned long)mm_kmalloc(KERNEL_STACK_SIZE);
  if (!stack){
    DEBUG("sys_call_fork can not alloc stack\n");
    ret = -ENOMEM;
    goto alloc_stack_error;
  }
  new_task->kernel_stack = stack + KERNEL_STACK_SIZE;
  memcpy((void *)stack, (void *)(cur_task->kernel_stack - KERNEL_STACK_SIZE), KERNEL_STACK_SIZE);
  new_task->remain_click = MAX_TASK_RUN_CLICK;
  new_task->fd_map = bitmap_clone(cur_task->fd_map);
  new_task->close_on_exec = bitmap_clone(cur_task->close_on_exec);
  if (!new_task->fd_map || !new_task->close_on_exec){
    DEBUG("sys_call_fork can not create bitmap\n");
    ret = -ENOMEM;
    goto bitmap_clone_error;
  }
  for (i = 0; i < MAX_OPEN_FD; i++)
    if (new_task->files[i])
      fs_get_file(new_task->files[i]);

  //mm_info should be new, and should use copy on write
  new_task->mm_info = task_vmm_clone_info(cur_task->mm_info);
  if (!new_task->mm_info){
    DEBUG("sys_call_fork can not clone vmm_info\n");
    ret = -ENOMEM;
    goto vmm_info_clone_error;
  }

  //signal_should be copy
  if (sig_task_fork(new_task, cur_task)){
    ret = -ENOMEM;
    goto sig_copy_error;
  }

  //update all count of inference
  task_get_bin(cur_task->bin);
  fs_get_file(cur_task->cur_dir);
  //setup task relationship
  task_adopt(cur_task, new_task);

  //add to manager list
  task_add_new_task(new_task);

  //make new_task scheduleable
  task_arch_init_run_context(new_task, 0);

  return new_task->pid;
  //error
 sig_copy_error:
  task_put_vmm_info(new_task->mm_info);
 vmm_info_clone_error:
  bitmap_destory(new_task->fd_map);
 bitmap_clone_error:
  mm_kfree((char*)stack);
 alloc_stack_error:
  slab_free_obj(new_task);
  return ret;
}

/*
 * Execve a new elf file.
 * This function will clear old vmm_info but not free it, then, the new content of
 * the vmm_info will be setup.
 * The fd with the flag of CLOSE_ON_EXCL will be closed.
 */
static int task_do_execve(const char *path, char * argv[], char * envp[])
{
  struct task * task = task_get_cur();
  struct fs_file * file;
  struct exec_bin * bin;
  int i;
  char * buf = (char *)mm_kmalloc(PAGE_SIZE);
  char * arg_buffer = (char *)mm_kmalloc(PAGE_SIZE);
  int ret = 0;
  char ** cur;
  char * cur_arg;
  unsigned long len;

  if (task_copy_str_from_user(buf, path, MAX_PATH_LEN)){
    ret = -EINVAL;
    goto get_path_error;
  }

  file = fs_open(buf, O_RDONLY, 0, &ret);
  if (!file)
    goto open_error;
  bin = elf_parse(file);
  if (!bin){
    ret = -EINVAL;
    goto elf_parse_error;
  }

  //Set up args
  if (argv){
    if (task_copy_pts_from_user(arg_buffer + 12, (void *)argv, MAX_ARG_NUM)){
      ret = -EINVAL;
      goto setup_args_error;
    }

    cur = (char **)(arg_buffer + 12);// not +16 since cur[0] is not argv[0] but path_name
    cur_arg = arg_buffer + PAGE_SIZE;
    //1.argv[]
    for (i = 0; i< MAX_ARG_NUM && cur[i]; i++){
      //buf is useable now
      if (task_copy_str_from_user(buf, cur[i], MAX_ARG_LEN)){
        ret = -EINVAL;
        goto setup_args_error;
      }

      len = strnlen(buf, MAX_ARG_LEN);
      cur_arg -= len + 1;
      memcpy(cur_arg, buf, len);
      cur_arg[len] = '\0';
      cur[i] = (char *)(TASK_USER_STACK_START - PAGE_SIZE + (cur_arg - arg_buffer));
    }
    //int main(int argc, char **argv)
    //3. we should setup argc  and argv
    //arg_buffer[0] is the return addr of user space _start
    //but it is not nessary to init arg_buffer[0]
    ((uint32 *)arg_buffer)[1] = i;
    ((uint32 *)arg_buffer)[2] = TASK_USER_STACK_START - PAGE_SIZE + 12;
  }
  //now we can free old vmm_info
  task_put_bin(task->bin);

  //rebuild mm_info
  task_vmm_clear(task->mm_info);
  task->bin = bin;

  if (task_init_bin_areas(task->mm_info, task->bin)
      || task_init_stack(task->mm_info, TASK_USER_STACK_START, TASK_USER_STACK_LEN)
      ||task_init_heap(task->mm_info, TASK_USER_HEAP_START, TASK_USER_HEAP_DEAULT_LEN)){
    ret = -EINVAL;
    goto init_area_error;
  }

  //copy to user space stack
  if (task_copy_to_user((void *)(TASK_USER_STACK_START - PAGE_SIZE), arg_buffer, PAGE_SIZE)){
    ret = -EINVAL;
    goto init_area_error;
  }
  //files
  for (i = 0; i < MAX_OPEN_FD; i++){
    if (task->files[i] && bitmap_check(task->close_on_exec, i)){
      bitmap_free(task->fd_map, i);
      bitmap_free(task->close_on_exec, i);
      fs_close(task->files[i]);
      task->files[i] = NULL;
    }
  }
  //signal
  sig_task_exec(task);
  mm_kfree(buf);
  mm_kfree(arg_buffer);
  task_arch_launch(task->bin->entry_addr, TASK_USER_STACK_START - PAGE_SIZE);
  //never back here

 init_area_error:
  task_put_vmm_info(task->mm_info);

 setup_args_error:
  task_put_bin(task->bin);
 elf_parse_error:
  fs_put_file(file);
 open_error:
 get_path_error:
  mm_kfree(arg_buffer);
  mm_kfree(buf);
  return ret;
}

/*
 * System call of execve.
 */
static int sys_call_execve(struct pt_regs * regs)
{
  const char * path_name = (const char *)sys_call_arg1(regs);
  char ** argv = (char **)sys_call_arg2(regs);
  char ** envp = (char **)sys_call_arg3(regs);
  return task_do_execve(path_name, argv, envp);
}

/*
 * This function move all the childs and zombie childs of "parent" to init task.
 * When a task exit but chlis still alive, all of it's chils will adopt to init task.
 */
static void task_adopt_orphans(struct task * parent)
{
  struct list_head * cur;
  struct list_head * next;
  struct task * child;
  list_for_each_safe(cur, next, &(parent->childs)){
    child = container_of(cur, struct task, child_list_entry);
    child->parent = init;
    list_move(cur, &(init->childs));
  }
  INIT_LIST_HEAD(&(parent->childs));

  list_for_each_safe(cur, next, &(parent->zombie_childs)){
    child = container_of(cur, struct task, child_list_entry);
    child->parent = init;
    list_move(cur, &(init->zombie_childs));
  }
  INIT_LIST_HEAD(&(parent->zombie_childs));
}

/*
 * This function make "parent" to wake up if "parent" is blocked for
 * system call of waitpid.
 */
static void task_exit_notify(struct task * parent)
{
  if (parent->waitpid_blocked)
    task_ready_to_run(parent);
}

/*
 * Exit a task.
 * This function will make a current task to be a zombie task.
 * A zombie task will free most of it's resources such as fd map and all the vmm area,
 * but it doesn't free some resources such as vmm_info untill it's parent call waitpid.
 */
void task_exit(int status)
{
  struct task * task = task_get_cur();
  struct task * parent = task->parent;
  assert(task->pid != 1);

  task->exit_status = status;
  task_tobe_zombie(task);
  task_leave_all_wq(task);
  task_adopt_orphans(task);
  //we should sure this is the only onwer of mm_info
  if (task->mm_info->count == 1)
    task_vmm_clear(task->mm_info);

  task_put_bin(task->bin);
  //close all file
  int i;
  for (i = 0; i < MAX_OPEN_FD; i++)
    if (task->files[i])
      fs_close(task->files[i]);
  bitmap_destory(task->fd_map);
  bitmap_destory(task->close_on_exec);
  fs_put_file(task->cur_dir);
  //delete signal
  sig_task_exit(task);
  //now we should link to parent zombie_chils list;
  list_del(&(task->child_list_entry));
  list_add(&(task->child_list_entry), &(parent->zombie_childs));
  //notify parent that there is new zombie chlid;
  task_exit_notify(parent);
  //give up cpu
  task_schedule();
  /** never back here **/
  DEBUG("exited task runing !\n");
  while (1);
}

/*
 * System call of exit.
 * Return 0 if successful or return error code if any error.
 */
static int sys_call_exit(struct pt_regs * regs)
{
  int status = (int)sys_call_arg1(regs);
  task_exit(status << 8);//POSIX defined...
  return 0;
}

/*
 * Clean all the resources of a task.
 * This function will free all the remain resource of a zombie task.
 * When this function is called, the task will be really deleted from system.
 */
static void task_clean_task(struct task * task)
{
  bitmap_free(task_map, task->pid);
  task_delete_task(task);
  task_put_vmm_info(task->mm_info);
  mm_kfree((void *)(task->kernel_stack - KERNEL_STACK_SIZE));
  slab_free_obj(task);
}

/*
 * System call of waitpid.
 * Check if there is any target zombie child and clean it.
 * If there is no zombie chlid at all, this function my block current task untill
 * task_exit_notity be called or a signal come.
 * Return 0 if successful or return error code if any error.
 */
static int sys_call_waitpid(struct pt_regs * regs)
{
  int pid = (int)sys_call_arg1(regs);
  int *status = (int *)sys_call_arg2(regs);
  struct task * cur_task = task_get_cur();
  struct list_head * cur;
  struct task * child = NULL;
  struct task * ret_child = NULL;
  int ret_pid;

  while (1){
    ret_child = NULL;
    if (pid > 0){ // wait for child which task.pid = pid;
      list_for_each(cur, &(cur_task->zombie_childs)){
        child = container_of(cur, struct task, child_list_entry);
        if (child->pid == pid){
          ret_child = child;
          break;
        }
      }
    }else if (pid == -1){ //wait for any child
      if (!list_empty(&(cur_task->zombie_childs))){
        ret_child = container_of(cur_task->zombie_childs.next, struct task,
                                   child_list_entry);
      }
    }else //error pid value
      return -EINVAL;
    if (ret_child)
      break;
    task_block(cur_task);
    cur_task->waitpid_blocked = 1;
    task_schedule();
    cur_task->waitpid_blocked = 0;

    if (sig_is_pending(cur_task)) //interrupted by signal
      return -EINTR;
  }

  list_del(&(ret_child->child_list_entry));
  if (task_copy_to_user((void *)status, (void *)(&(ret_child->exit_status)), sizeof(int))){
    return -EFAULT;
  }
  ret_pid = ret_child->pid;
  task_clean_task(ret_child);
  return ret_pid;
}

/*
 * System call of brk.
 * Change the end address of user heap area.
 * Return 0 if successful or return error code if any error.
 */
static int sys_call_brk(struct pt_regs * regs)
{
  unsigned long addr = (unsigned long)sys_call_arg1(regs);
  struct task * task = task_get_cur();
  if (addr < task->mm_info->heap->start_addr ||
      addr > task->mm_info->stack->start_addr - task->mm_info->stack->len)
    return -EINVAL;
  task->mm_info->heap->len = addr - task->mm_info->heap->start_addr;
  return 0;
}

/*
 * System call of sbrk.
 * Increase length of user heap area.
 * Return old end address of heap area or return error code if any error.
 */
static int sys_call_sbrk(struct pt_regs * regs)
{
  int incre = (int)sys_call_arg1(regs);
  struct task* task = task_get_cur();
  unsigned long cur_end;

  cur_end = task->mm_info->heap->len + task->mm_info->heap->start_addr;
  if (cur_end + incre > task->mm_info->stack->start_addr)
    return -EINVAL;
  task->mm_info->heap->len += incre;
  return cur_end;

}

/*
 * System call of chdir.
 * Change the current work dir of current task.
 * Current dir is the start dir of the Relative path.
 * Return 0 if successful or return error code if any error.
 */
static int sys_call_chdir(struct pt_regs * regs)
{
  const char * path = (const char *)sys_call_arg1(regs);
  struct task * task = task_get_cur();
  char * tmp_buffer = (char *)mm_kmalloc(MAX_PATH_LEN + 1);
  struct fs_file * file;
  int ret = -1;

  if (task_copy_str_from_user(tmp_buffer, path, MAX_PATH_LEN))
    goto copy_error;
  file = fs_open(tmp_buffer, O_RDONLY, 0, &ret);
  if (!file)
    goto open_error;
  if (!S_ISDIR(file->inode->mode)){
    ret = -ENOTDIR;
    goto open_error;
  }

  fs_put_file(task->cur_dir);
  task->cur_dir = file;
  mm_kfree(tmp_buffer);
  return 0;

 open_error:
 copy_error:
  mm_kfree(tmp_buffer);
  return ret;
}

/*
 * System call of getpid.
 * Return pid of current task.
 * This function always success.
 */
static int sys_call_getpid(struct pt_regs * regs)
{
  return task_get_cur()->pid;
}

/*
 * Constructor of "struct task_wait_qaueue".
 */
static void wait_queue_constr(void *arg)
{
  struct task_wait_queue * queue = (struct task_wait_queue *)arg;
  INIT_LIST_HEAD(&(queue->entry_list));
}

/*
 * Initate task life cycle management.
 */
void task_init()
{
  task_arch_init();
  task_cache = slab_create_cache(sizeof(struct task), task_constr, NULL, "task cache");
  assert(task_cache);

  bin_cache = slab_create_cache(sizeof(struct exec_bin), exec_bin_constr, exec_bin_distr, "bin cache");
  assert(bin_cache);

  section_cache = slab_create_cache(sizeof(struct section), NULL, NULL, "section cache");
  assert(section_cache);

  wait_entry_cache = slab_create_cache(sizeof(struct task_wait_entry), NULL, NULL, "wait_entry cache");
  assert(wait_entry_cache);

  wait_queue_cache = slab_create_cache(sizeof(struct task_wait_queue), wait_queue_constr, NULL, "wait_queue cache");
  assert(wait_queue_cache);

  task_vmm_init();
  task_schedule_init();
  task_map = bitmap_create(MAX_PID_NUM);
  bitmap_alloc(task_map); //give up pid 0
  sys_call_init();

  //regist sys_call
  sys_call_regist(SYS_CALL_FORK, sys_call_fork);
  sys_call_regist(SYS_CALL_EXIT, sys_call_exit);
  sys_call_regist(SYS_CALL_EXECVE, sys_call_execve);
  sys_call_regist(SYS_CALL_WAITPID, sys_call_waitpid);
  sys_call_regist(SYS_CALL_SBRK, sys_call_sbrk);
  sys_call_regist(SYS_CALL_GETPID,sys_call_getpid);
  sys_call_regist(SYS_CALL_BRK, sys_call_brk);
  sys_call_regist(SYS_CALL_CHDIR, sys_call_chdir);
}

/*
 * Setup the first task named init.
 * This function nerver return.
 */
void task_setup_init(const char* path)
{
  int ret = -1;
  struct fs_file * file = fs_open(path, O_RDONLY, 0, &ret);
  if (!file){
    DEBUG("can not open %s\n", path);
    return ;
  }

  init = slab_alloc_obj(task_cache);
  if (!init)
    goto task_alloc_error;

  init->fd_map = bitmap_create(MAX_OPEN_FD);
  init->close_on_exec = bitmap_create(MAX_OPEN_FD);
  if (!init->fd_map || !init->close_on_exec)
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
      || task_init_heap(init->mm_info, TASK_USER_HEAP_START, TASK_USER_HEAP_DEAULT_LEN))
    goto init_mm_error;

  //setup cur_dir, stdin, stdout, stderr
  init->cur_dir = fs_open("/", O_RDWR, 0, &ret);
  if (ret)
    goto open_root_error;

  if (sig_task_init(init))
    goto init_signal_error;

  bitmap_set(init->fd_map, 0);
  bitmap_set(init->fd_map, 1);
  bitmap_set(init->fd_map, 2);
  init->files[0] = fs_open_stdin();
  init->files[1] = fs_open_stdout();
  init->files[2] = fs_open_stderr();
  //stdin, stdout and stderr should not be closed on execve

  //for schedule
  init->state = TASK_STATE_RUN;
  init->remain_click = MAX_TASK_RUN_CLICK;
  task_add_new_task(init);

  task_arch_befor_launch(init);
  //this function never return
  task_arch_launch(init->bin->entry_addr, init->mm_info->stack->start_addr + init->mm_info->stack->len - 12);
  /******** never back here ***********************/

 init_signal_error:
  fs_close(init->cur_dir);
 open_root_error:
 init_mm_error:
  task_put_bin(init->bin);
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

/*
 * Alloc exec_bin or section.
 */
struct exec_bin * task_new_exec_bin()
{
  return slab_alloc_obj(bin_cache);
}

struct section * task_new_section()
{
  return slab_alloc_obj(section_cache);
}

/*
 * Alloc new task_wait_queue or task_wait_entry.
 */
struct task_wait_queue * task_new_wait_queue()
{
  return slab_alloc_obj(wait_queue_cache);
}

struct task_wait_entry * task_new_wait_entry()
{
  return slab_alloc_obj(wait_entry_cache);
}

/*
 * Add a wait_entry to a wait_queue.
 * A task can wait on multi queue at the same time.
 */
void task_wait_on(struct task_wait_entry * entry,struct task_wait_queue* queue)
{
  uint32 save = arch_irq_save();
  arch_irq_disable();
  list_add_tail(&(entry->wait_list_entry), &(queue->entry_list));
  list_add_tail(&(entry->task_we_entry), &(entry->task->wait_e_list));
  arch_irq_recover(save);
}

/*
 * Remove wait_entry from the wait_queue which it waited on.
 */
void task_leave_from_wq(struct task_wait_entry* wait_entry)
{
  uint32 save = arch_irq_save();
  arch_irq_disable();
  list_del(&(wait_entry->wait_list_entry));
  list_del(&(wait_entry->task_we_entry));
  arch_irq_recover(save);
}

/*
 * Make current task leave from all the wait_queue that it wait on.
 * A task can wait on multi queue at the same time.
 */
void task_leave_all_wq(struct task* task)
{
  struct list_head * cur, * next;
  struct task_wait_entry * entry;

  list_for_each_safe(cur, next, &(task->wait_e_list)){
    entry = container_of(cur,struct task_wait_entry, task_we_entry);
    task_leave_from_wq(entry);
  }
}

/*
 * Wake up one of the task waitting on the queue.
 */
void task_notify_one(struct task_wait_queue *queue)
{
  struct task_wait_entry * entry;
  if (list_empty(&(queue->entry_list)))
      return ;
  entry = container_of(queue->entry_list.next,
                                                struct task_wait_entry,
                                                wait_list_entry);
  if (entry->wake_up)
    entry->wake_up(entry->task, entry->private);
}

/*
 * Wake up all the task waitting on the queue.
 */
void task_notify_all(struct task_wait_queue * queue)
{
  struct list_head * cur;
  struct task_wait_entry * entry;
  list_for_each(cur, &(queue->entry_list)){
    entry = container_of(cur, struct task_wait_entry, wait_list_entry);
    if (entry->wake_up)
      entry->wake_up(entry->task, entry->private);
  }
}

/*
 * Task segment fault.
 * This function will exit task.
 */
void task_segment_fault(struct task * task)
{
  printk("segment fault!\n");
  sig_send(task,SIGSEGV);
}

/*
 * A gerner function for wake up.
 * Every wait_entry has it's wake_up function, but mostly, the function just call
 * task_ready_to_run, so, this function can be used to do that.
 */
void task_gener_wake_up(struct task* task,void* private)
{
  task_ready_to_run(task);
}
