/*************************************************
 *   Author: Ray Huang
 *   Date  : 2017/5/10
 *   Email : rayhuang@126.com
 *   Desc  : errno , perror
 ************************************************/
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
const char *const sys_errlist[] =
{
    [0] = "Success",
    [1] = "Operation not permitted",
    [2] = "No such file or directory",
    [3] = "No such process",
# 48 "../sysdeps/gnu/errlist.c"
    [4] = "Interrupted system call",
    [5] = "Input/output error",
# 62 "../sysdeps/gnu/errlist.c"
    [6] = "No such device or address",
    [7] = "Argument list too long",
    [8] = "Exec format error",
    [9] = "Bad file descriptor",
    [10] = "No child processes",
    [11] = "Resource temporarily unavailable",
    [12] = "Cannot allocate memory",
    [13] = "Permission denied",
    [14] = "Bad address",
    [15] = "Block device required",
    [16] = "Device or resource busy",
    [17] = "File exists",
    [18] = "Invalid cross-device link",
    [19] = "No such device",
    [20] = "Not a directory",
    [21] = "Is a directory",
    [22] = "Invalid argument",
    [24] = "Too many open files",
    [23] = "Too many open files in system",
    [25] = "Inappropriate ioctl for device",
    [26] = "Text file busy",
    [27] = "File too large",
    [28] = "No space left on device",
    [29] = "Illegal seek",
    [30] = "Read-only file system",
    [31] = "Too many links",
    [32] = "Broken pipe",
    [33] = "Numerical argument out of domain",
    [34] = "Numerical result out of range",
};

int err_num = 0;

int * __errno_location()
{
    return &err_num;
}

void perror(const char *s)
{
    printf("%s\n", s);
    printf("%s\n", sys_errlist[errno]);
}
