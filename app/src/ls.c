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
#include <sys/stat.h>
#include <stdlib.h>
#include <sys/ioctl.h>
int flag_l;

void show_stat(struct stat * stat)
{
  char mode_c[] = "__________";

  if (S_ISDIR(stat->st_mode))
    mode_c[0] = 'd';
  if (S_IRUSR & stat->st_mode)
    mode_c[1] = 'r';
  if (S_IWUSR & stat->st_mode)
    mode_c[2] = 'w';
  if (S_IXUSR & stat->st_mode)
    mode_c[3] = 'x';
  if (S_IRGRP & stat->st_mode)
    mode_c[4] = 'r';
  if (S_IWGRP & stat->st_mode)
    mode_c[5] = 'w';
  if (S_IXGRP & stat->st_mode)
    mode_c[6] = 'x';
  if (S_IROTH &stat->st_mode)
    mode_c[7] = 'r';
  if (S_IWOTH &stat->st_mode)
    mode_c[8] = 'w';
  if (S_IXOTH &stat->st_mode)
    mode_c[9] = 'x';
  printf("%s %4d ray ray %10d ", mode_c, stat->st_nlink, stat->st_size);
}

void show_one(const char * name, struct stat * stat)
{
  if (flag_l)
    show_stat(stat);
  if (S_ISDIR(stat->st_mode))
    ioctl(0, 4, 0x6);
  printf("%s", name);
  if (S_ISDIR(stat->st_mode))
    printf("/");

  printf("\n");
  ioctl(0, 4, 0x7);
}
void show_dir(const char * name)
{
  char path[PATH_MAX + 1];
  DIR * dir = opendir(name);
  int fd;
  struct dirent * dirent;
  struct stat statbuf;
  if (!dir){
    perror("ls failed!");
    exit(1);
  }

  while ((dirent = readdir(dir))){
    sprintf(path, "%s/%s", name, dirent->d_name);
    fd = open(path, O_RDONLY);
    if (fd < 0){
      perror("ls failed!");
      exit(1);
    }
    if (fstat(fd, &statbuf) < 0){
      perror("ls failed!");
      exit(1);
    }
    show_one(dirent->d_name, &statbuf);
    close(fd);
  }
  closedir(dir);
}


int main(int argc, char **argv)
{
  const char * path;
  struct stat statbuf;
  int opt;
  while ((opt = getopt(argc, argv, "l")) != -1){
    switch(opt){
    case 'l':
      flag_l = 1;
      break;
    case '?':
      printf("Usage: ls [-l] [dir]\n");
      return 1;
    }
  }
  if (argc == optind)
    path = "./";
  else
    path = argv[optind];
  if (stat(path, &statbuf) < 0){
    perror("ls failed!");
    return 1;
  }

  if (S_ISDIR(statbuf.st_mode))
    show_dir(path);
  else
    show_one(path, &statbuf);
  return 0;
}
