/*************************************************
 *   Author: Ray Huang
 *   Date  : 2017/5/7
 *   Email : rayhuang@126.com
 *   Desc  :
 ************************************************/
#include <fcntl.h>
#include <stdio.h>
int main(int argc, char **argv)
{
  if (argc != 2){
    printf("Usage: touch [filename]\n");
    return 1;
  }
  open(argv[1], O_RDONLY | O_CREAT | O_EXCL, 0666);
  return 0;
}
