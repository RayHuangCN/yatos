/*
 *  RAM memory management
 *
 *  Copyright (C) 2017 ese@ccnt.zju
 *
 *  ---------------------------------------------------
 *  Started at 2017/4/4 by Ray
 *
 *  ---------------------------------------------------
 *
 *  This file is subject to the terms and conditions of the GNU General Public
 *  License.
 */

#ifndef __YATOS_MM_H
#define __YATOS_MM_H

#include <arch/system.h>
#include <yatos/list.h>
#include <yatos/pmm.h>
#include <yatos/slab.h>

#define MAX_KMALLOC_SLAB_LEVE 11
#define MIN_KMALLOC_SLAB_LEVE 4

#define paddr_to_vaddr(paddr) (KERNEL_VMM_START + ((paddr) - PHY_MM_START))
#define paddr_to_page(paddr) pmm_paddr_to_page(paddr)
#define vaddr_to_paddr(vaddr) (PHY_MM_START + ((vaddr) - KERNEL_VMM_START))
#define vaddr_to_page(vaddr) pmm_paddr_to_page(vaddr_to_paddr(vaddr))

void mm_init();
void * mm_kmalloc(unsigned long size);
void mm_kfree(void *addr);

#endif /* __YATOS_MM_H */
