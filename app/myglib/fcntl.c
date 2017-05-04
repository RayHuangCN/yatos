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

int open(const char* __file,int __oflag,...)
{
  va_list arg;
  va_start(arg, __oflag);
  mode_t mode = va_arg(arg, mode_t);
  va_end(arg);
  return sys_call_4(SYS_CALL_OPEN,
                    (unsigned long)__file,
                    (unsigned long)__oflag,
                    (unsigned long)mode);
}

int ioctl(int __fd,unsigned long int __request, ...)
{
  va_list arg;
  va_start(arg, __request);
  unsigned long req_arg = va_arg(arg, unsigned long);
  va_end(arg);

  return sys_call_4(SYS_CALL_IOCTL,
                    (unsigned long)__fd,
                    __request,
                    req_arg);
}


ssize_t read(int __fd,void* __buf,size_t __nbytes)
{
  return sys_call_4(SYS_CALL_READ, __fd,
                    (unsigned long)__buf,
                    (unsigned long)__nbytes);
}

ssize_t write(int __fd,const void* __buf,size_t __n)
{
  return sys_call_4(SYS_CALL_WRITE,__fd,
                    (unsigned long)__buf,
                    (unsigned long)__n);
}
int close(int __fd)
{
  return sys_call_2(SYS_CALL_CLOSE, __fd);
}

off_t lseek(int __fd,__off_t __offset,int __whence)
{
  return sys_call_4(SYS_CALL_SEEK, __fd,
                    (unsigned long)__offset,
                    (unsigned long)__whence);
}

int fsync(int __fd)
{
  return sys_call_2(SYS_CALL_SYNC, __fd);
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
