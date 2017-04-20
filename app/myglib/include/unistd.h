#ifndef __MYGLIB_UNISTD_H
#define __MYGLIB_UNISTD_H

/*************************************************
 *   Author: Ray Huang
 *   Date  : 2017/4/13
 *   Email : rayhuang@126.com
 *   Desc  : tiny unistd.h
 ************************************************/
typedef int pid_t;

pid_t  fork();
void _exit(int status);


#endif
