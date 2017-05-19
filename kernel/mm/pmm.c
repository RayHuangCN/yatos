/*
 *  Physical memory page management.
 *
 *  Copyright (C) 2017 ese@ccnt.zju
 *
 *  ---------------------------------------------------
 *  Started at 2017/4/ by Ray
 *
 *  ---------------------------------------------------
 *
 *  This file is subject to the terms and conditions of the GNU General Public
 *  License.
 */

#include <arch/system.h>
#include <yatos/pmm.h>
#include <yatos/printk.h>
#include <yatos/tools.h>
#include <yatos/list.h>

static struct page pmm_pages[FREE_PAGE_TOTAL];
static struct list_head pmm_free_page_lists[PMM_MAX_LEVE];
static unsigned long pmm_free_lists_count[PMM_MAX_LEVE];
static unsigned long pmm_useable_page = FREE_PAGE_TOTAL;

/*
 * Show memory utilization
 */
void pmm_show_useable()
{
  int i;
  printk("total memory : %dMB and %dKB\n\r", PHY_MM_SIZE / 1024 / 1024,
         (PHY_MM_SIZE / 1024) % 1024);
  printk("free useable memory : %dMB and %dKB\n\r",
         (pmm_useable_page * 4) / 1024 ,
         (pmm_useable_page * 4) % 1024);

  for (i = 0; i < PMM_MAX_LEVE; ++i)
    if (pmm_free_lists_count[i])
      printk("%d KB = %u\n\r", (1 << i) * 4 , pmm_free_lists_count[i]);
}

/*
 * Insert pages to "list".
 * We need to keep "list" in no-descending order.
 */
static void pmm_insert_to(struct list_head *list, struct page * page)
{
  struct page * cur_page = NULL;
  struct list_head * pos = NULL;
  unsigned long size = 1 << (list - pmm_free_page_lists);
  int overlap;

  list_for_each(pos, list){
    cur_page = container_of(pos, struct page, page_list);

    overlap = (cur_page <= page && cur_page + size > page) ||
      (cur_page >= page && page + size > cur_page);
    assert(!overlap);

    if (cur_page > page){
      list_add_tail(&(page->page_list), &(cur_page->page_list));
     return ;
    }
  }
  list_add_tail(&(page->page_list), list);
  pmm_free_lists_count[list - pmm_free_page_lists]++;
}

/*
 * Putback pages to free lists.
 * Pages may be split and insert to different list.
 */
static void pmm_putback_remain(struct page* remain, unsigned long size)
{
  struct page * temp = NULL;
  int pow = 0;

  if (size == 0)
    return ;
  temp = remain;
  for (pow = 0; pow < PMM_MAX_LEVE; ++pow){
    if (((size >> pow) & 1)){
      pmm_insert_to(pmm_free_page_lists + pow, temp);
      temp += 1 << pow;
    }
  }
}

/*
 * Physical memory management init.
 * This function will be called only by mm_init.
 * Just insert all free pages into free lists.
 */
void pmm_init()
{
  int i;
  for (i = 0; i < PMM_MAX_LEVE; ++i)
    INIT_LIST_HEAD(pmm_free_page_lists + i);
  pmm_putback_remain(pmm_pages, FREE_PAGE_TOTAL);
}

/*
 * Address change functions.
 */
unsigned long pmm_page_to_paddr(struct page * page){
  return FREE_PMM_START + ((page - pmm_pages) << PAGE_SHIFT);
}
struct page * pmm_paddr_to_page(unsigned long address){
  return pmm_pages + (address - FREE_PMM_START) / PAGE_SIZE;
}

/*
 * Alloc pages.
 * This function is not the interface of other modules since we need do something else if
 * we failed to alloc pages at first time.
 * So, this function may be called more than once when other modules try to alloc pages.
 * Return the first page if successful or return NULL if no pages found.
 *
 * Note: "align" is the power of 2, but "size" is not.
 */
static struct  page * pmm_do_alloc(unsigned long size, unsigned long align)
{
  struct page * cur = NULL, * ret = NULL;
  unsigned long start_addr, align_addr, block_size, end_addr;
  struct list_head * pos = NULL;
  int i;

  align = (1 << align) * PAGE_SIZE;
  for (i = 0; i < PMM_MAX_LEVE; i++){
    block_size = 1 << i;
    if (size > block_size)
      continue;

    cur = NULL;
    list_for_each(pos, pmm_free_page_lists + i){
      cur = container_of(pos, struct page, page_list);

      //check for align
      start_addr = pmm_page_to_paddr(cur);
      end_addr = pmm_page_to_paddr(cur + block_size);
      align_addr = start_addr + (start_addr % align);
      ret = pmm_paddr_to_page(align_addr);

      //check size
      if (block_size - (ret - cur) < size)
        continue;

      //get it
      list_del(&(cur->page_list));
      pmm_free_lists_count[i]--;
      pmm_putback_remain(cur, ret - cur);
      pmm_putback_remain(ret + size, block_size - (ret - cur) - size);
      return ret;
    }
  }
  return NULL;
}

/*
 * Arrange free lists.
 * Merge adjacent pages in the same free list that address are continuous in order to get lager page block.
 */
static void pmm_do_arrange()
{
  struct list_head * cur_list = NULL;
  struct page * cur_pages = NULL, * next_pages = NULL;
  struct list_head * cur = NULL, * temp = NULL;
  int i;
  for (i = 0; i < PMM_MAX_LEVE; ++i){
    cur_list = pmm_free_page_lists + i;

    for (cur = cur_list->next;
         cur != cur_list && cur->next != cur_list;
         /* nor */ ){
      cur_pages = container_of(cur, struct page, page_list);
      next_pages = container_of(cur->next, struct page, page_list);

      temp = cur->next;
      if ((cur_pages + (1 << i)) == next_pages){
        temp = temp->next;
        list_del(cur);
        list_del(cur->next);
        pmm_insert_to(cur_list + 1, cur_pages);
      }
      cur = temp;
    }
  }
}

/*
 * The interface of other modules to alloc pages.
 * If it failed to alloc pages at first time, it will call pmm_do_arrange and try again
 * Return first page if successful or return NULL if no pages found.
 */
struct page * pmm_alloc_pages(unsigned long size, unsigned long align)
{
  struct page * ret = pmm_do_alloc(size,align);
  int i;

  if (!ret){
    pmm_do_arrange();
    ret = pmm_do_alloc(size,align);
  }

  if (ret){
    pmm_useable_page -= size;
    for (i = 0; i < size; ++i){
      ret[i].count = 1;
      ret[i].type = PMM_PAGE_TYPE_NORMAL;
    }
  }
  return ret;
}

/*
 * Free pages that alloc from pmm_alloc_pages.
 */
void pmm_free_pages(struct page * pages, unsigned long size)
{
  int i;
  pmm_putback_remain(pages,size);
  for (i = 0; i < size; ++i){
    pages[i].count = 0;
    pages[i].private = NULL;
  }
  pmm_useable_page += size;
}
