/*************************************************
 *   Author: Ray Huang
 *   Date  : 2017/4/20
 *   Email : rayhuang@126.com
 *   Desc  : stdlib
 ************************************************/
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>

extern unsigned char __bss_start;
extern unsigned char _edata;
extern unsigned char _end;
extern int main(int argc, char **argv);

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


void exit(int __status)
{
  _exit(__status);
}
