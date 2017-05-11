/*************************************************
 *   Author: Ray Huang
 *   Date  :
 *   Email : rayhuang@126.com
 *   Desc  :
 ************************************************/
#include <stdio.h>
#include <unistd.h>
#include <error.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <string.h>
#include <stdlib.h>

int main(int argc, char **argv)
{
  pid_t child;
  int fd[2];
  if (pipe(fd)){
    perror("pipe error!");
    return 1;
  }
  child  = fork();
  if (child < 0){
    perror("fork error!");
    return 1;
  }
  //parent
  if (child){
    close(fd[1]);
    char buffer[128];
    int n;
    while ((n = read(fd[0], buffer, 128)) > 0)
      write(STDOUT_FILENO, buffer, n);
  }
  //child
  else{
    close(fd[0]);
    int i;
    for (i = 0; i < 3; i++){
      usleep(2000000);//2second
      write(fd[1], "child\n", strlen("child\n") + 1);
    }
  }
  return 0;
}
