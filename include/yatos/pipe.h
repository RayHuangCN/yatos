#ifndef __YATOS_PIPE_H
#define __YATOS_PIPE_H

/*************************************************
 *   Author: Ray Huang
 *   Date  : 2017/5/10
 *   Email : rayhuang@126.com
 *   Desc  : PIPE
 ************************************************/
#include <arch/system.h>
#include <yatos/task.h>

#define PIPE_BUFFER_SIZE PAGE_SIZE

struct pipe_info
{
  int read_offset;
  int write_offset;
  int data_size;
  char *buffer;
  int reader_count;
  int writer_count;
  struct task_wait_queue * r_wait_queue;
  struct task_wait_queue * w_wait_queue;
};

void pipe_init();


#endif
