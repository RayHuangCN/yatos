/*************************************************
 *   Author: Ray Huang
 *   Date  : 2017/5/4
 *   Email : rayhuang@126.com
 *   Desc  : ls
 ************************************************/
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <sys/types.h>

int main(int argc, char **argv)
{
  const char * path;
  if (argc == 1)
    path = "./";
  else
    path = argv[1];
  DIR * dir = opendir(path);
  if (!dir){
    printf("can not open %s\n", path);
    return 1;
  }
  struct dirent * dirent;
  while ((dirent = readdir(dir))){
      printf("%s\n", dirent->d_name);
  }
  closedir(dir);
  return 0;
}
