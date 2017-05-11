/*************************************************
 *   Author: Ray Huang
 *   Date  : 2017/4/23
 *   Email : rayhuang@126.com
 *   Desc  : stdio file
 ************************************************/
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

static int std_write(struct fs_file * file, char *buffer, unsigned long count)
{
  unsigned long i;
  for (i = 0; i < count; i++)
    putc(buffer[i]);
  return i;
}

static int std_read(struct fs_file * file, char *buffer, unsigned long count)
{
  return tty_read_input(buffer, count);
}

static int std_ioctl(struct fs_file * file, int requst,  unsigned long arg)
{
  switch (requst){
  case 1: // get tty max number
    return MAX_TTY_NUM;
  case 2: // open tty, if there is no useable tty, return -1
    (task_get_cur()->tty_num = tty_open_new());
    if (task_get_cur()->tty_num < 0)
      return -ENOTTY;
    return 0;
  case 3:
    tty_clear(); // clear
    return 0;
  case 4: //set color
    tty_set_color(arg);
    return 0;
  }
  return -EINVAL;
}

static struct fs_inode_oper stdin_oper = {
  .read = std_read,
  .ioctl = std_ioctl
};

static struct fs_inode_oper stdout_oper = {
  .write = std_write,
  .ioctl = std_ioctl
};

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
