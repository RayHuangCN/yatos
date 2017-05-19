/*
 *  some defination of helpful functions
 *
 *  Copyright (C) 2017 ese@ccnt.zju
 *
 *  ---------------------------------------------------
 *  Started at 2017/3/30 by Ray
 *
 *  ---------------------------------------------------
 *
 *  This file is subject to the terms and conditions of the GNU General Public
 *  License.
 */

#ifndef __YATOS_TOOLS_H
#define __YATOS_TOOLS_H

#include <arch/system.h>
#include <yatos/printk.h>

#define offsetof(TYPE, MEMBER) ((size_t) &((TYPE *)0)->MEMBER)
#define container_of(ptr, type, member) ({                      \
	const typeof( ((type *)0)->member ) *__mptr = (ptr);    \
	(type *)( (char *)__mptr - offsetof(type,member) );})

#define ALIGN(va, align) ((unsigned long)(va) + (align - 1) / align * align)

/*======================== DEBUG ==============================*/
#define DEBUG_ON 1
#if (DEBUG_ON == 1)
#define DEBUG(str, args...) printk(str, ##args)
#else
#define DEBUG(str, args...)
#endif

#if (DEBUG_ON == 1)
#define assert(cond) \
  if (!(cond)){      \
    printk("["#cond"] failed");\
    printk("%s\n", __FILE__);\
    printk("%d: %s\n", __LINE__, __FUNCTION__);\
    while(1);\
  }
#else
#define assert(cond)
#endif

#endif /* __YATOS_TOOLS_H */
