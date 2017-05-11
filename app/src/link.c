/*************************************************
 *   Author: Ray Huang
 *   Date  : 2017/5/8
 *   Email : rayhuang@126.com
 *   Desc  : link
 ************************************************/
#include <unistd.h>
#include <stdio.h>
int main(int argc, char **argv)
{
  if (argc != 3){
    printf("Usage: link [oldpath] [newpath]\n");
    return 1;
  }
  if (link(argv[1], argv[2]) == -1){
    perror("link failed!\n");
    return 1;
  }

  return 0;
}
