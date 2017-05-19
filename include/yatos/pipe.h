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

#ifndef __YATOS_PIPE_H
#define __YATOS_PIPE_H

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
#endif /* __YATOS_PIPE_H */
