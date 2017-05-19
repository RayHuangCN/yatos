/*
 *  Files for stdio.
 *  That is, read from tty and write to tty.
 *
 *  Copyright (C) 2017 ese@ccnt.zju
 *
 *  ---------------------------------------------------
 *  Started at 2017/4/23 by Ray
 *
 *  ---------------------------------------------------
 *
 *  This file is subject to the terms and conditions of the GNU General Public
 *  License.
 */

#include <yatos/fs.h>
#include <yatos/tty.h>
#include <yatos/printk.h>
#include <yatos/mm.h>
#include <yatos/task.h>
#include <yatos/task_vmm.h>
#include <yatos/schedule.h>
#include <yatos/errno.h>

static struct fs_file *stdin;
static struct fs_file *stdout;

/*
 * Write function of std inode, buffer will be writed to the tty that current task use.
 * Return write count.
 */
static int std_write(struct fs_file * file, char *buffer, unsigned long count)
{
  unsigned long i;
  for (i = 0; i < count; i++)
    putc(buffer[i]);
  return i;
}

/*
 * Read function of std inode, read from tty that current task use.
 * Return read count if successful or return -EINTR if get a signal.
 *
 * Note: this function will block current task until read a '\n' or buffer is full or a signal
         has been sent to current task.
 */
static int std_read(struct fs_file * file, char *buffer, unsigned long count)
{
  return tty_read_input(buffer, count);
}

/*
 * Ioctl function of std inode. Task use can this function to contrl tty.
 * Return 0 or MAX_TTY_NUM if successful or return error code if any error.
 *
 * Note requst:
 *       1 = get tty max number
 *       2 = open new tty
 *       3 = clear tty
 *       4 = set color, arg = color
 *       5 = change fg task, arg = task pid
 */
static int std_ioctl(struct fs_file * file, int requst,  unsigned long arg)
{
  struct task * t_task;
  switch (requst){
  case 1: // get tty max number
    return MAX_TTY_NUM;
  case 2:
    if (tty_open_new(task_get_cur()))
      return -ENOTTY;
    return 0;
  case 3:
    tty_clear(); // clear
    return 0;
  case 4: //set color
    tty_set_color(arg);
    return 0;
  case 5: //change fg_task
    t_task = task_find_by_pid(arg);
    if (!t_task)
      return -EINVAL;
    return tty_set_fg_task(task_get_cur()->tty_num, task_get_cur(), t_task);
  }
  return -EINVAL;
}

/*
 * Stdio inode operations.
 */
static struct fs_inode_oper stdin_oper = {
  .read = std_read,
  .ioctl = std_ioctl
};

static struct fs_inode_oper stdout_oper = {
  .write = std_write,
  .ioctl = std_ioctl
};

/*
 * Get stdio fs files.
 */
struct fs_file * fs_open_stdin()
{

  if (!stdin){
    stdin = fs_new_file();
    struct fs_inode * inode = fs_new_inode();
    inode->action = &stdin_oper;
    stdin->inode = inode;
  }else
    fs_get_file(stdin);
  return stdin;
}

struct fs_file * fs_open_stdout()
{
  if (!stdout){
    stdout = fs_new_file();
    struct fs_inode * inode = fs_new_inode();
    inode->action = &stdout_oper;
    stdout->inode = inode;
  }else
    fs_get_file(stdout);
  return stdout;
}

struct fs_file * fs_open_stderr()
{
  return fs_open_stdout();
}
