/*************************************************
 *   Author: Ray Huang
 *   Date  : 2017/4/13
 *   Email : rayhuang@126.com
 *   Desc  : test
 ************************************************/
#include <unistd.h>
#include <stdio.h>
#include <sys/wait.h>
pid_t child;
int status;
int main(int argc, char **argv)
{
  if ((child = fork())){
    printf("init fork child = %d\n", child);
    while (1){
      printf("wait for any child\n");
      child = waitpid(-1, &status, 0);
      printf("child = %d exit_status = %d\n", child, WEXITSTATUS(status));
    }
  }else{
    //child
    execve("/test", NULL, NULL);
    printf("after execve\n");
  }
  return 0;
}
