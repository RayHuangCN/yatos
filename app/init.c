/*************************************************
 *   Author: Ray Huang
 *   Date  : 2017/4/13
 *   Email : rayhuang@126.com
 *   Desc  : test
 ************************************************/
#include <unistd.h>
#include <stdio.h>
pid_t child;

char *ags[] = {"1", "2", "3", "4", NULL};
char *evs[] = {"A", "B", "C", "D", NULL};


int main(int argc, char **argv)
{
  if ((child = fork())){
    //parent
    printf("child id = %d\n", child);
    while (1);
  }else{
    //child
    printf("befor execve\n");
    execve("/test", ags, evs);
    printf("after execve\n");
    while (1);
  }
  return 0;
}
