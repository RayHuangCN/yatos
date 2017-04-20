#ifndef __YATOS_SCHEDULE_H
#define __YATOS_SCHEDULE_H

/*************************************************
 *   Author: Ray Huang
 *   Date  : 2017/4/20
 *   Email : rayhuang@126.com
 *   Desc  : schedule manager
 ************************************************/
#include <yatos/task.h>

#define MAX_TASK_RUN_CLICK 5

void task_schedule();
void task_schedule_init();
void task_add_new_task(struct task *new);
void task_ready_to_run(struct task *task);
struct task*  task_get_cur();

void task_check_schedule();

#endif
