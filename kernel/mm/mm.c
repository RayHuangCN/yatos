/*************************************************
 *   Author: Ray Huang
 *   Date  : 2017/4/4
 *   Email : rayhuang@126.com
 *   Desc  : memory manager
 ************************************************/
#include <yatos/mm.h>
#include <yatos/pmm.h>
#include <yatos/tools.h>
#include <yatos/printk.h>
#include <arch/mmu.h>
struct kcache * kmalloc_caches[MAX_KMALLOC_SLAB_LEVE];

static void kmalloc_init()
{
  int i;
  char name[32];
  for (i = MIN_KMALLOC_SLAB_LEVE; i < MAX_KMALLOC_SLAB_LEVE; ++i){
    sprintf(name, "kmalloc cache %d", i);
    kmalloc_caches[i] = slab_create_cache(1 << i, NULL, NULL, name);
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
  mmu_init();
  pmm_init();
  slab_init();
  kmalloc_init();
}

void * mm_kmalloc(unsigned long size)
{
  struct page * ret_page;
  int i;
  if (!size)
    return NULL;

  for (i = MIN_KMALLOC_SLAB_LEVE ; i < MAX_KMALLOC_SLAB_LEVE; ++i)
    if (size <= (1 << i))
      return (void *)slab_alloc_obj(kmalloc_caches[i]);

  size = (size + PAGE_SIZE - 1) / PAGE_SIZE;

  ret_page = pmm_alloc_pages(size,0);
  if (!ret_page)
    return NULL;

  ret_page->type = PMM_PAGE_TYPE_KMALLOC;
  ret_page->use_for.kmalloc_info.size = size;

  return (void *)paddr_to_vaddr(pmm_page_to_paddr(ret_page));
}

void mm_kfree(void * addr)
{
  struct page * page = vaddr_to_page((unsigned long)addr);

  if (page->type == PMM_PAGE_TYPE_SLAB)
    slab_free_obj(addr);
  else
    pmm_free_pages(page, page->use_for.kmalloc_info.size);
}


struct page*  vaddr_to_page(unsigned long vaddr)
{
  unsigned long paddr = vaddr_to_paddr(vaddr);
  return pmm_paddr_to_page(paddr);
}
