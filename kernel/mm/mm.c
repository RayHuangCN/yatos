/*
 *  RAM memory management, but this file not include the detail of page manage and slab
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

#include <yatos/mm.h>
#include <yatos/pmm.h>
#include <yatos/tools.h>
#include <yatos/printk.h>
#include <arch/mmu.h>

static struct kcache * kmalloc_caches[MAX_KMALLOC_SLAB_LEVE];

/*
 * Initate the kcaches of common size slab.
 */
static void kmalloc_init()
{
  int i;
  char name[32];
  for (i = MIN_KMALLOC_SLAB_LEVE; i < MAX_KMALLOC_SLAB_LEVE; ++i){
    sprintf(name, "kmalloc cache %d", i);
    kmalloc_caches[i] = slab_create_cache(1 << i, NULL, NULL, name);
    assert(kmalloc_caches[i]);
  }
}

/*
 * Initate RAM manage module.
 */
void mm_init()
{
  mmu_init();
  pmm_init();
  slab_init();
  kmalloc_init();
}

/*
 * Alloc memory from memory management.
 * we use slab_alloc_obj or directly use pmm_alloc_pages according to "size".
 * Return useable address if successful or return NULL if any error.
 */
void * mm_kmalloc(unsigned long size)
{
  struct page * ret_page;
  int i;
  assert(size);

  for (i = MIN_KMALLOC_SLAB_LEVE ; i < MAX_KMALLOC_SLAB_LEVE; ++i)
    if (size <= (1 << i))
      return (void *)slab_alloc_obj(kmalloc_caches[i]);

  //how many pages we need
  size = (size + PAGE_SIZE - 1) / PAGE_SIZE;
  ret_page = pmm_alloc_pages(size,0);
  if (!ret_page)
    return NULL;
  ret_page->type = PMM_PAGE_TYPE_KMALLOC;
  ret_page->use_for.kmalloc_info.size = size;

  return (void *)paddr_to_vaddr(pmm_page_to_paddr(ret_page));
}

/*
 * Free memory that we alloced before by mm_kmalloc.
 * we use slab_free_obj or pmm_free_pages according to the type of page struct,
 * it had been initiated when we use mm_kmalloc.
 */
void mm_kfree(void * addr)
{
  if (!addr)
    return ;

  struct page * page = vaddr_to_page((unsigned long)addr);
  assert(page->type == PMM_PAGE_TYPE_KMALLOC || page->type == PMM_PAGE_TYPE_SLAB);

  if (page->type == PMM_PAGE_TYPE_SLAB)
    slab_free_obj(addr);
  else
    pmm_free_pages(page, page->use_for.kmalloc_info.size);
}
