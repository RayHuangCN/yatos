/*************************************************
 *   Author: Ray Huang
 *   Date  : 2017/4/23
 *   Email : rayhuang@126.com
 *   Desc  : open read wirte close lseek
 ************************************************/
#include "sys_call.h"
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <stdarg.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <sys/stat.h>

struct kstat
{
  int inode_num;
  mode_t mode;
  int links_count;
  off_t size;
};

int fcntl(int __fd,int __cmd,...)
{
  va_list arg;
  va_start(arg, __cmd);
  int flag = va_arg(arg, int);
  va_end(arg);
  int ret;
  ret = sys_call_4(SYS_CALL_FCNTL,
                   (unsigned long)__fd,
                   (unsigned long)__cmd,
                   (unsigned long)flag);
  if (ret < 0){
    errno = -ret;
    return -1;
  }
  return ret;
}

int dup(int __fd)
{
  return fcntl(__fd, F_DUPFD);
}

int dup3(int __fd,int __fd2,int __flags)
{
  int ret;
  ret = sys_call_4(SYS_CALL_DUP3,
                   (unsigned long)__fd,
                   (unsigned long)__fd2,
                   (unsigned long)__flags);
  if (ret < 0){
    errno = -ret;
    return -1;
  }
  return ret;
}


int open(const char* __file,int __oflag,...)
{
  va_list arg;
  va_start(arg, __oflag);
  mode_t mode = va_arg(arg, mode_t);
  va_end(arg);
  int ret;
  ret = sys_call_4(SYS_CALL_OPEN,
                    (unsigned long)__file,
                    (unsigned long)__oflag,
                    (unsigned long)mode);
  if (ret  < 0){
    errno = -ret;
    return -1;
  }
  return ret;
}

int ioctl(int __fd,unsigned long int __request, ...)
{
  va_list arg;
  va_start(arg, __request);
  unsigned long req_arg = va_arg(arg, unsigned long);
  va_end(arg);
  int ret;
  ret = sys_call_4(SYS_CALL_IOCTL,
                    (unsigned long)__fd,
                    __request,
                    req_arg);
  if (ret  < 0){
    errno = -ret;
    return -1;
  }
  return ret;
}


ssize_t read(int __fd,void* __buf,size_t __nbytes)
{
  int ret;
  ret = sys_call_4(SYS_CALL_READ, __fd,
                    (unsigned long)__buf,
                    (unsigned long)__nbytes);
  if (ret  < 0){
    errno = -ret;
    return -1;
  }
  return ret;
}

ssize_t write(int __fd,const void* __buf,size_t __n)
{
  int ret;
  ret = sys_call_4(SYS_CALL_WRITE,__fd,
                    (unsigned long)__buf,
                    (unsigned long)__n);
  if (ret  < 0){
    errno = -ret;
    return -1;
  }
  return ret;
}
int close(int __fd)
{
  int ret;
  ret =  sys_call_2(SYS_CALL_CLOSE, __fd);
  if (ret  < 0){
    errno = -ret;
    return -1;
  }
  return ret;
}

off_t lseek(int __fd,__off_t __offset,int __whence)
{
  int ret;
  ret = sys_call_4(SYS_CALL_SEEK, __fd,
                    (unsigned long)__offset,
                    (unsigned long)__whence);
  if (ret  < 0){
    errno = -ret;
    return -1;
  }
  return ret;
}

int fsync(int __fd)
{
  int ret;
  ret = sys_call_2(SYS_CALL_SYNC, __fd);
  if (ret  < 0){
    errno = -ret;
    return -1;
  }
  return ret;
}

int truncate(const char* __file,__off_t __length)
{
  int ret;
  int fd = open(__file , O_RDWR);
  if (fd < 0)
    return -1;
  ret = ftruncate(fd, __length);
  close(fd);
  return ret;
}

int ftruncate(int __fd,__off_t __length)
{
  int ret;
  ret = sys_call_3(SYS_CALL_FTRUNCATE,
                    (unsigned long)__fd,
                    (unsigned long)__length);
  if (ret  < 0){
    errno = -ret;
    return -1;
  }
  return ret;
}
int stat(const char *pathname, struct stat *statbuf)
{
  int ret;
  int fd = open(pathname, O_RDWR);
  if (fd < 0)
    return -1;
  ret = fstat(fd, statbuf);
  close(fd);
  return ret;
}

int fstat(int fd, struct stat * statbuf)
{
  int ret;
  struct kstat kstat;
  ret = sys_call_3(SYS_CALL_FSTAT,
                   (unsigned long)fd,
                   (unsigned long)&kstat);
  if (ret < 0){
    errno = -ret;
    return -1;
  }
  statbuf->st_ino = kstat.inode_num;
  statbuf->st_size = kstat.size;
  statbuf->st_nlink = kstat.links_count;
  statbuf->st_mode = kstat.mode;
  return ret;
}


int getc()
{
  int n;
  char c;
  n = read(0, &c, 1);

  if (n > 0)
    return c;
  else
    return n;
}

void putc(char c)
{
  write(1, &c, 1);
}
