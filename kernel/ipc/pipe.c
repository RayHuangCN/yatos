/*
 *  Pipe flie operation
 *
 *  Copyright (C) 2017 ese@ccnt.zju
 *
 *  ---------------------------------------------------
 *  Started at 2017/5/10 by Ray
 *
 *  ---------------------------------------------------
 *
 *  This file is subject to the terms and conditions of the GNU General Public
 *  License.
 */

#include <yatos/pipe.h>
#include <arch/regs.h>
#include <yatos/sys_call.h>
#include <yatos/mm.h>
#include <yatos/task.h>
#include <yatos/schedule.h>
#include <yatos/fs.h>
#include <yatos/bitmap.h>
#include <yatos/errno.h>

/*
 * Read function of pipe inode.
 * Read data from pipe data buffer, if there is no data in data buffer, this function
 * will block current task or return error code if there is no any writter.
 * This function will wake up any wirtter which is waitting for data buffer free space.
 * Return read count if successful or return error code if any error.
 */
static int pipe_read(struct fs_file * file, char * buffer, unsigned long count)
{
  struct pipe_info * pipe_info;
  struct task_wait_entry entry;
  int read_high, read_low, read_max;
  struct task * task;

  task = task_get_cur();
  pipe_info = file->inode->inode_data;
  entry.task = task;
  entry.wake_up = task_gener_wake_up;

  if (!count)
    return 0;
  while (1){
    read_max = pipe_info->data_size < count ? pipe_info->data_size : count;
    if (read_max){
      if (pipe_info->read_offset + read_max > PIPE_BUFFER_SIZE){
        read_high = PIPE_BUFFER_SIZE - pipe_info->read_offset;
        read_low = pipe_info->read_offset + read_max - PIPE_BUFFER_SIZE;
      }else{
        read_high = read_max;
        read_low = 0;
      }
      if (task_copy_to_user(buffer, pipe_info->buffer + pipe_info->read_offset, read_high))
        return -EINVAL;
      if (read_low && task_copy_to_user(buffer + read_high, pipe_info->buffer, read_low))
        return -EINVAL;
      pipe_info->data_size -= read_max;
      pipe_info->read_offset = (pipe_info->read_offset + read_max) % PIPE_BUFFER_SIZE;

      //now we should notify w_wait_queue
      task_notify_all(pipe_info->w_wait_queue);

      return read_max;
    }else{
      //no data
      //we should wait for data or return 0 if there is no writer any more
      if (!pipe_info->writer_count)
        return 0; //no writer , we don't block here
      //wait on r_wait_queue
      task_block(task);
      task_wait_on(&entry, pipe_info->r_wait_queue);
      task_schedule();
      task_leave_from_wq(&entry);
    }
  }
  return 0;
}

/*
 * Write function of pipe inode.
 * Write data to pipe data buffer, if there is no free space in data buffer, this function
 * will block current task or return error code if there is no any reader.
 * This function will wake up any reader which is waitting for data.
 * Return write count if successful or return error code if any error.
 */
static int pipe_write(struct fs_file * file, char *buffer, unsigned long count)
{
  struct pipe_info * pipe_info;
  struct task_wait_entry entry;
  struct task * task;
  int write_max, write_high, write_low, remain_space;

  task = task_get_cur();
  pipe_info = file->inode->inode_data;
  entry.task = task;
  entry.wake_up = task_gener_wake_up;
  remain_space = PIPE_BUFFER_SIZE - pipe_info->data_size;

  if (!count)
    return 0;

  while (1){
    write_max = remain_space < count ? remain_space : count;
    if (write_max){
      if (pipe_info->write_offset + write_max > PIPE_BUFFER_SIZE){
        write_high = PIPE_BUFFER_SIZE - pipe_info->write_offset;
        write_low = pipe_info->write_offset + write_max - PIPE_BUFFER_SIZE;
      }else{
        write_high = write_max;
        write_low = 0;
      }
      if (task_copy_from_user(pipe_info->buffer + pipe_info->write_offset, buffer, write_high))
        return -EINVAL;
      if (write_low && task_copy_from_user(pipe_info->buffer, buffer + write_high, write_low))
        return -EINVAL;

      pipe_info->data_size += write_max;
      pipe_info->write_offset = (pipe_info->write_offset + write_max) % PIPE_BUFFER_SIZE;

      //notify reader to wake up
      task_notify_all(pipe_info->r_wait_queue);
      return write_max;
    }else{
      //if there is no reader , we don't block
      if (!pipe_info->reader_count)
        return 0;

      //wait for space
      task_block(task);
      task_wait_on(&entry, pipe_info->w_wait_queue);
      task_schedule();
      task_leave_from_wq(&entry);
    }
  }
  return 0;
}

/*
 * Relase function of pipe inode.
 * Decrease writer count or reader count, if there is no writer and no reader,
 * this function will free all memory of this pipe.
 * This function may wake up task which is waitting for data or waitting for free space of data
 * buffer.
 */
static void pipe_release(struct fs_inode * inode)
{
  struct pipe_info * pipe_info = inode->inode_data;
  if (inode->inode_num){
    pipe_info->writer_count--;
    task_notify_all(pipe_info->r_wait_queue);
  }
  else{
    pipe_info->reader_count--;
    task_notify_all(pipe_info->w_wait_queue);
  }

  //pipe_info should be release
  if (!pipe_info->reader_count && !pipe_info->writer_count){
    mm_kfree(pipe_info->buffer);
    task_free_wait_queue(pipe_info->r_wait_queue);
    task_free_wait_queue(pipe_info->w_wait_queue);
    mm_kfree(pipe_info);
  }
}

/*
 * Pipe inode action.
 */
struct fs_inode_oper pipe_reader_action = {
  .read = pipe_read,
  .release = pipe_release
};
struct fs_inode_oper pipe_writer_action = {
  .write = pipe_write,
  .release = pipe_release
};

/*
 * System call of pipe.
 * Setup a new pipe.
 * A pipe inclues a read file which is associated to fd[0], and a write file
 * which is associated to fd[1].
 * Return 0 and fill sys_call_arg1 if successful or return error code if any error.
 */
static int sys_call_pipe(struct pt_regs * regs)
{
  int *fd = (int*)sys_call_arg1(regs);
  int kfd[2]; //kfd[0] = read, kfd[1] = write
  struct task * task = task_get_cur();
  struct fs_file * file[2];
  struct fs_inode * inode[2];
  struct pipe_info * pipe_info = NULL;
  int ret = 0;

  kfd[0] = bitmap_alloc(task->fd_map);
  kfd[1] = bitmap_alloc(task->fd_map);
  if (kfd[0] < 0 || kfd[1] < 0)
    return -ENFILE;

  file[0] = fs_new_file();
  file[1] = fs_new_file();
  inode[0] = fs_new_inode();
  inode[1] = fs_new_inode();
  if (!file[0] || !file[1] || !inode[0] || !inode[1]){
    ret = -ENOMEM;
    goto err;
  }

  pipe_info = mm_kmalloc(sizeof(*pipe_info));
  if (!pipe_info){
    ret = -ENOMEM;
    goto err;
  }

  pipe_info->reader_count = 1;
  pipe_info->writer_count = 1;

  pipe_info->buffer = mm_kmalloc(PIPE_BUFFER_SIZE);
  if (!pipe_info->buffer){
    ret = -ENOMEM;
    goto err;
  }

  pipe_info->r_wait_queue = task_new_wait_queue();
  pipe_info->w_wait_queue = task_new_wait_queue();
  if (!pipe_info->r_wait_queue || !pipe_info->w_wait_queue){
    ret = -ENOMEM;
    goto err;
  }

  inode[0]->count = 1;
  inode[0]->inode_num = 0; //means this is reader inode
  inode[0]->action = &pipe_reader_action;
  inode[0]->inode_data = pipe_info;
  file[0]->flag = O_RDONLY;
  file[0]->inode = inode[0];
  task->files[kfd[0]] = file[0];

  inode[1]->count = 1;
  inode[1]->inode_num = 1; //mean this is writer inode
  inode[1]->action = & pipe_writer_action;
  inode[1]->inode_data = pipe_info;
  file[1]->flag = O_WRONLY;
  file[1]->inode = inode[1];
  task->files[kfd[1]] = file[1];

  if (task_copy_to_user(fd, kfd, sizeof(kfd))){
    ret = -EFAULT;
    goto err;
  }
  return 0;
 err:
  if (inode[0])
    fs_put_inode(inode[0]);
  if (inode[1])
    fs_put_inode(inode[1]);
  if (file[0])
    fs_put_file(file[0]);
  if (file[1])
    fs_put_file(file[1]);
  if (fd[0] >= 0)
    bitmap_free(task->fd_map, fd[0]);
  if (fd[1] >= 0)
    bitmap_free(task->fd_map, fd[1]);

  if (pipe_info){
    if (pipe_info->buffer)
      mm_kfree(pipe_info->buffer);
    if (pipe_info->r_wait_queue)
      task_free_wait_queue(pipe_info->r_wait_queue);
    if (pipe_info->w_wait_queue)
      task_free_wait_queue(pipe_info->w_wait_queue);
    mm_kfree(pipe_info);
  }
  return ret;
}

void pipe_init()
{
  sys_call_regist(SYS_CALL_PIPE, sys_call_pipe);
}
