#ifndef __ARCH_MM_H
#define __ARCH_MM_H

/*************************************************
 *   Author: Ray Huang
 *   Date  : 2017/4/19
 *   Email : rayhuang@126.com
 *   Desc  : mm low level
 ************************************************/
#include <arch/system.h>
#include <arch/regs.h>

void mm_arch_init();
void mm_arch_flush_mmu();
#endif
