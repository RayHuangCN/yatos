/*************************************************
 *   Author: Ray Huang
 *   Date  : 2017/4/4
 *   Email : rayhuang@126.com
 *   Desc  : some common use function
 ************************************************/
#include <yatos/printk.h>
#include <yatos/tools.h>
void go_die(const char * info)
{
  printk(info);
  while (1);
}
