#ifndef __YATOS_TIMER_H
#define __YATOS_TIMER_H

/*************************************************
 *   Author: Ray Huang
 *   Date  : 2017/4/2
 *   Email : rayhuang@126.com
 *   Desc  : timer interface
 ************************************************/

#include <yatos/list.h>

#define TIMER_HZ 100


struct timer_action
{
  struct list_head list_entry;
  void *private;
  unsigned long target_click;
  void (*action)(void *private);
};

void timer_init();
int timer_register(struct timer_action * action);
void timer_unregister(struct timer_action * action);
unsigned long timer_get_click();
void timer_action_init(struct timer_action * action);

int timer_usleep(unsigned long usec);
#endif
