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

#define MAX_KMALLOC_SLAB_LEVE 11
#define MIN_KMALLOC_SLAB_LEVE 4

void mm_init();


unsigned long paddr_to_vaddr(unsigned long paddr);
unsigned long vaddr_to_paddr(unsigned long vaddr);
struct page * vaddr_to_page(unsigned long vaddr);

void * mm_kmalloc(unsigned long size);
void mm_kfree(void *addr);


void * mm_vmalloc(unsigned long size);
void mm_vfree(void  *addre);



#endif
