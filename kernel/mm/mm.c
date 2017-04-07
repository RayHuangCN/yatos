/*************************************************
 *   Author: Ray Huang
 *   Date  : 2017/4/4
 *   Email : rayhuang@126.com
 *   Desc  : memory manager
 ************************************************/
#include <yatos/mm.h>
#include <yatos/pmm.h>
#include <yatos/tools.h>

struct kcache * kmalloc_caches[MAX_KMALLOC_SLAB_LEVE];

static void kmalloc_init()
{
  int i;
  for (i = 0; i < MAX_KMALLOC_SLAB_LEVE; ++i){
    kmalloc_caches[i] = slab_create_cache(1 << i, NULL, NULL);
    if (!kmalloc_caches[i])
      go_die("can not init kmalloc caches!\n\r");
  }
}

unsigned long paddr_to_vaddr(unsigned long paddr)
{
  return KERNEL_VMM_START + (paddr - PHY_MM_START);
}


unsigned long vaddr_to_paddr(unsigned long vaddr)
{
  return PHY_MM_START + (vaddr - KERNEL_VMM_START);
}


void mm_init()
{
  pmm_init();
  slab_init();
  kmalloc_init();
}

unsigned long mm_kmalloc(unsigned long size)
{
  struct page * ret_page;
  int i;
  if (!size)
    return 0;

  for (i = 0 ; i < MAX_KMALLOC_SLAB_LEVE; ++i)
    if (size < (1 << i))
      return slab_alloc_obj(kmalloc_caches[i]);

  size = (size + PAGE_SIZE - 1) / PAGE_SIZE;

  ret_page = pmm_alloc_pages(size,0);
  if (!ret_page)
    return 0;
  return paddr_to_vaddr(pmm_page_to_paddr(ret_page));
}

void mm_kfree(unsigned long addre)
{
