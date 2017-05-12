#include <sys/types.h>
#include <dirent.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include "sys_call.h"
#include <string.h>
#include <stdio.h>

struct kdir
{
  int fd;
  struct dirent * dirent;
};

struct kdirent
{
  int inode_num;
  int name_len;
  char name[256];
};

DIR * opendir(const char* __name)
{

  struct kdir * ret;
  int fd = open(__name, O_RDONLY);
  if (fd < 0)
    return NULL;
  ret = (struct kdir *)malloc(sizeof(struct kdir));
  ret->fd = fd;
  ret->dirent = (struct dirent*)malloc(sizeof(struct dirent));
  return (DIR*)ret;
}

struct dirent * readdir(DIR* __dirp)
{
  if (!__dirp)
    return NULL;

  struct kdir * kdir = (struct kdir*)__dirp;
  struct dirent * dirent = (struct dirent*)kdir->dirent;
  struct kdirent kdirent;
  int ret = sys_call_3(SYS_CALL_READDIR, kdir->fd, &kdirent);
  if (!ret){
    dirent->d_ino = kdirent.inode_num;
    strcpy(dirent->d_name, kdirent.name);
    return dirent;
  }
  return NULL;
}

int closedir(DIR* __dirp)
{
  if (!__dirp)
    return 0;
  struct kdir * kdir = (struct kdir*)__dirp;
  if (close(kdir->fd))
    return 1;
  free(kdir->dirent);
  free(kdir);
}

void rewinddir(DIR* __dirp)
{
  if (!__dirp)
    return ;
  struct kdir * kdir = (struct kdir*)__dirp;
  lseek(kdir->fd, 0, SEEK_SET);
}
