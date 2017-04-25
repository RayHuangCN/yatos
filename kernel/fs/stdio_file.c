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
  unsigned long i;
  for (i = 0; i < count; i++)
    buffer[i] = getc();
  return i;
}

static struct fs_inode_oper stdin_oper = {
  .read = std_read
};

static struct fs_inode_oper stdout_oper = {
  .write = std_write
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
