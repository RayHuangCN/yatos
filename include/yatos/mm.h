#ifndef __YATOS_MM_H
#define __YATOS_MM_H

/*************************************************
 *   Author: Ray Huang
 *   Date  : 2017/4/4
 *   Email : rayhuang@126.com
 *   Desc  : memory interface and defines
 ************************************************/
#include <arch/system.h>
#include <yatos/list.h>
#include <yatos/pmm.h>
#include <yatos/slab.h>

#define MAX_KMALLOC_SLAB_LEVE 8


void mm_init();


unsigned long paddr_to_vaddr(unsigned long paddr);
unsigned long vaddr_to_paddr(unsigned long vaddr);


unsigned long mm_kmalloc(unsigned long size);
void mm_kfree(unsigned long addre);


unsigned long mm_vmalloc(unsigned long size);
void mm_vfree(unsigned long addre);



#endif
