/*************************************************
 *   Author: Ray Huang
 *   Date  : 2017/4/13
 *   Email : rayhuang@126.com
 *   Desc  : start file
 ************************************************/
#include <unistd.h>

void _start(int argc, char **argv)
{
  //clean bss
  unsigned char *cur = &__bss_start;
  unsigned char *end = &_end;
  while (cur < end){
    *cur = 0;
    cur++;
  }
  int ret = main(argc, argv);
  exit(ret);
}
