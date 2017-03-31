#ifndef __TTY_H
#define __TTY_H

/*************************************************
 *   Author: Ray Huang
 *   Date  : 2017/3/31
 *   Email : rayhuang@126.com
 *   Desc  : console control
 ************************************************/

/********* g_function ***************************/
struct tty_cursor
{
  int x;
  int y;
};

void tty_set_cursor(struct tty_cursor * cursor);
void tty_get_cursor(struct tty_cursor * ret);
void tty_reset_cursor();
void tty_clear();
#endif
