/*************************************************
 *   Author: Ray Huang
 *   Date  : 2017/4/13
 *   Email : rayhuang@126.com
 *   Desc  : test
 ************************************************/
#include <unistd.h>
#include <stdio.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <sys/ioctl.h>
int tty_max_num;
int main(int argc, char **argv)
{
  pid_t shell;
  tty_max_num = ioctl(0, 1, 0);
  if (tty_max_num <= 0)
    return 1;

  int i;
  for (i = 0; i < tty_max_num; i++){
    shell = fork();
    if (shell < 0)
      return 1;

    if (shell == 0){
      if (ioctl(0, 2, 0) < 0) // open new tty
        return 1;
      execve("sbin/shell", NULL, NULL);
      printf("can not execve shell\n");
      return 1;
    }
  }

  while (1){
    int ret;
    waitpid(-1, &ret, 0);
  }
  return 0;
}
