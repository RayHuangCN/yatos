#ifndef __MYGLIB_UNISTD_H
#define __MYGLIB_UNISTD_H

/*************************************************
 *   Author: Ray Huang
 *   Date  : 2017/4/13
 *   Email : rayhuang@126.com
 *   Desc  : tiny unistd.h
 ************************************************/
#include <exit.h>


extern unsigned char __bss_start;
extern unsigned char _edata;
extern unsigned char _end;
extern int main(int argc, char **argv);
#endif
