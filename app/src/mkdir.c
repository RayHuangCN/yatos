/*************************************************
 *   Author: Ray Huang
 *   Date  : 201/5/7
 *   Email : rayhuang@126.com
 *   Desc  : mkdir
 ************************************************/
#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
int main(int argc, char **argv)
{
  if (argc != 2){
    printf("Usage: mkdir [pathname]\n");
    return 1;
  }

  if (mkdir(argv[1], 0777) < 0){
    perror("mkdir failed!");
    return 1;
  }

  return 0;
}
