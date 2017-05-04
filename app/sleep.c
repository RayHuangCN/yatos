/*************************************************
 *   Author: Ray Huang
 *   Date  : 2017/5/5
 *   Email : rayhuang@126.com
 *   Desc  : sleep
 ************************************************/
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

int main(int argc, char **argv)
{
  if (argc != 2){
    printf("Usage sleep [second]\n");
    return 1;
  }
  int time = 0;
  sscanf(argv[1], "%d", &time);
  usleep(time * 1000000);
  return 0;
}
