#ifndef __YATOS_TASK_H
#define __YATOS_TASK_H

/*************************************************
 *   Author: Ray Huang
 *   Date  : 2017/4/4
 *   Email : rayhuang@126.com
 *   Desc  : task manager interface
 ************************************************/
#define TASK_STATE_RUN 1
#define TASK_STATE_STOP 2

struct task
{
  unsigned long state;
  unsigned long pid;
  unsigned long ppid;
  struct list_head task_list_entry;
  unsigned long remain_click;
}




#endif
