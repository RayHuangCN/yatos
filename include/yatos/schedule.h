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
#define TASK_STATE_RUN 1
#define TASK_STATE_BLOCK 2
#define TASK_STATE_ZOMBIE 3



void task_schedule();
void task_schedule_init();
void task_add_new_task(struct task *new);
void task_delete_task(struct task * task);

void task_tobe_zombie(struct task * task);
void task_ready_to_run(struct task *task);
void task_block(struct task * task);


struct task*  task_get_cur();
void task_check_schedule();
struct task * task_find_by_pid(int pid);




#endif