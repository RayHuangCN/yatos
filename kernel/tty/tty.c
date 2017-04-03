/*************************************************
 *   Author: Ray Huang
 *   Date  : 2017/3/31
 *   Email : rayhuang@126.com
 *   Desc  : tty control
 ************************************************/

/********* header files *************************/
#include <yatos/tty.h>
#include <arch/vga.h>
/********* g_function ***************************/
void tty_set_cursor(struct tty_cursor * cursor)
{
  vga_set_cursor(cursor->y * 80 + cursor->x);
}


void tty_get_cursor(struct tty_cursor * ret)
{
  unsigned short cur = vga_get_cursor();
  ret->y = cur / 80;
  ret->x = cur % 80;
}


void tty_reset_cursor(void)
{
  struct tty_cursor tc;
  tc.x = 0;
  tc.y = 0;
  tty_set_cursor(&tc);
}

void tty_clear(void)
{
  tty_reset_cursor();
  vga_clean();
}

void tty_init()
{
  tty_reset_cursor();
  tty_clear();
}
