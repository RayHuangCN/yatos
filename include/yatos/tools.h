#ifndef __YATOS_H
#define __YATOS_H

/*************************************************
 *   Author: Ray Huang
 *   Date  : 2017/4/2
 *   Email : rayhuang@126.com
 *   Desc  : tools function or define
 ************************************************/

/********* header files *************************/
#include <arch/system.h>
#include <printk/string.h>

#define offsetof(TYPE, MEMBER) ((size_t) &((TYPE *)0)->MEMBER)
#define container_of(ptr, type, member) ({                      \
	const typeof( ((type *)0)->member ) *__mptr = (ptr);    \
	(type *)( (char *)__mptr - offsetof(type,member) );})


void go_die(const char *);



#endif
