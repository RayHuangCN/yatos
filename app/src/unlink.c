/*************************************************
 *   Author: Ray Huang
 *   Date  : 2017/5/8
 *   Email : rayhuang@126.com
 *   Desc  : unlink
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
  if (argc != 2){
    printf("Usage: unlink [path]\n");
    return 1;
  }
  if (unlink(argv[1]) == -1){
    perror("unlink failed!\n");
    return 1;
  }
  return 0;
}
