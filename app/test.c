/*************************************************
 *   Author: Ray Huang
 *   Date  : 2017/4/13
 *   Email : rayhuang@126.com
 *   Desc  : test
 ************************************************/
#include <unistd.h>

void sys_call()
{
  asm("int $0x80\n");
}

int main(int argc, char **argv)
{
  while (1){
    sys_call();
  }
}
