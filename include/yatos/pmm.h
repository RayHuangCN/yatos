#ifndef __YATOS_PMM_H
#define __YATOS_PMM_H

/*************************************************
 *   Author: Ray Huang
 *   Date  : 2017/4/5
 *   Email : rayhuang@126.com
 *   Desc  : physic memory manager interface
 ************************************************/
#include <arch/system.h>
#include <yatos/list.h>

#define PMM_MAX_LEVE 32
#define PMM_TOTAL_PAGE PHY_MM_SIZE / PAGE_SIZE


#define pmm_get_pages(page, size) (++(page->count))
#define pmm_put_pages(page, size) \
  if (!(--(page->count)))         \
    pmm_free_pages(page, size);


#define pmm_alloc_one() pmm_alloc_pages(1, 0)
#define pmm_free_one(page) pmm_free_pages(page, 1)
#define pmm_get_one(page) pmm_get_pages(page, 1)
#define pmm_put_one(page) pmm_put_pages(page, 1)

struct page
{
  unsigned long type;
  unsigned long count;
  unsigned long vaddress;
  struct list_head page_list;
  struct slab_frame
  {
    struct list_head first_free_obj;
    unsigned long free_obj_num;
  }slab;
};

void pmm_init();
struct page* pmm_alloc_pages(unsigned long size, unsigned long align);
void pmm_free_pages(struct page * pages, unsigned long size);

unsigned long pmm_page_to_paddr(struct page *);
struct page * pmm_paddr_to_page(unsigned long address);
void pmm_show_usable();




#endif
