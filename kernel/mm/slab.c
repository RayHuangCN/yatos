/*
 *  Slab memory management.
 *
 *  Copyright (C) 2017 ese@ccnt.zju
 *
 *  ---------------------------------------------------
 *  Started at 2017/4/5 by Ray
 *
 *  ---------------------------------------------------
 *
 *  This file is subject to the terms and conditions of the GNU General Public
 *  License.
 */

#include <yatos/printk.h>
#include <yatos/mm.h>
#include <yatos/slab.h>
#include <yatos/list.h>
#include <printk/string.h>

static struct kcache kcache_cache; //The kache for manage all kcaches
static struct list_head kcache_list; // All created kcaches

/*
 * The constructor for the object of "struct kcache".
 * No destructor is needed.
 */
void slab_kcache_constr(void * kcache)
{
  struct kcache * kc = (struct kcache *)kcache;
  INIT_LIST_HEAD(&(kc->full_cache));
  INIT_LIST_HEAD(&(kc->part_cache));
}

/*
 * Initate slab memory management.
 */
void slab_init()
{
  INIT_LIST_HEAD(&kcache_list);
  slab_kcache_constr(&kcache_cache);
  kcache_cache.constr = slab_kcache_constr;
  kcache_cache.obj_size = sizeof(struct kcache);
}

/*
 * Create a slab cache.
 * "constr" and "distr" are allowed to be NULL, but name is not.
 * Return new kcache if successful or return NULL if any error.
 */
struct kcache * slab_create_cache(unsigned long size,cache_constr_fun constr,cache_distr_fun distr,
                                  const char * name)
{
  struct kcache * ret_kc;

  assert(name);

  //size is not allowed to less than 16
  if (size < 16)
    size = 16;

  ret_kc = slab_alloc_obj(&kcache_cache);
  if (!ret_kc){
    DEBUG("can not alloc kcache\n");
    return NULL;
  }

  ret_kc->constr = constr;
  ret_kc->distr = distr;
  ret_kc->obj_size = size;
  strncpy(ret_kc->name, name, 32);
  list_add_tail(&(ret_kc->kc_list_entry), &kcache_list);

  return ret_kc;
}

/*
 * Alloc a new page to increase userful space of "cache".
 * All object in new page will be linked into free list.
 * Return initated page if successful or return NULL if no page.
 */
static struct page * slab_get_new_page(struct kcache * cache)
{
  struct page * new_page;
  struct list_head *cur_node;
  unsigned long cur_addr;
  int i;

  new_page = pmm_alloc_one();
  if (!new_page){
    DEBUG("get NULL page in slab_get_new_page");
    return NULL;
  }

  new_page->type = PMM_PAGE_TYPE_SLAB;
  page_to_slab(new_page)->parent = cache;
  INIT_LIST_HEAD(&(page_to_slab(new_page)->free_list));

  cur_addr = paddr_to_vaddr(pmm_page_to_paddr(new_page));
  for (i = 0;i < objs_per_page(cache); i++){
    cur_node = (struct list_head *)cur_addr;
    list_add_tail(cur_node, &(page_to_slab(new_page)->free_list));
    cur_addr += cache->obj_size;
  }

  list_add_tail(&(new_page->page_list), &(cache->part_cache));

  return new_page;
}

/*
 * Alloc a object from "cache".
 * This function will auto alloc new page if there is not free node.
 * Return useable object address if successful or return NULL if any error.
 */
void * slab_alloc_obj(struct kcache* cache)
{
  struct list_head * ret_page_list;
  struct page * ret_page;
  struct list_head * ret_obj;
  struct page * new_page;

  //if there is no free node, we should alloc more page.
  if (list_empty(&(cache->part_cache))){
    new_page = slab_get_new_page(cache);
    if (!new_page){
      DEBUG("get new page error\n\r");
      return NULL;
    }
  }

  //get the first free node in the first page of part_cache of cache;
  ret_page_list = cache->part_cache.next;
  ret_page = container_of(ret_page_list, struct page, page_list);
  ret_obj = page_to_slab(ret_page)->free_list.next;

  list_del(ret_obj);

  //if there is no free node in ret_page now,
  //we need to move ret_page to full_list of cache.
  if (list_empty(&(page_to_slab(ret_page)->free_list))){
    list_del(ret_page_list);
    list_add_tail(ret_page_list, &(cache->full_cache));
  }
  memset(ret_obj, 0, cache->obj_size);

  if (cache->constr)
    cache->constr((void *)ret_obj);

  return (void *)ret_obj;
}

/*
 * Free object that alloced by slab_alloc_obj.
 * Note: this function never free page even if the page has no alloced objects any more.
 */
void slab_free_obj(void* obj)
{
  unsigned long addr;
  struct list_head * obj_list;
  struct page * page;
  struct kcache * cache;

  if (!obj)
    return ;
  addr = (unsigned long)obj;
  obj_list = (struct list_head*)addr;
  page = vaddr_to_page(PAGE_ALIGN(addr));
  cache = page_to_slab(page)->parent;

  if (cache->distr)
    cache->distr(obj);

  if (list_empty(&(page_to_slab(page)->free_list))){
    list_del(&(page->page_list));
    list_add_tail(&(page->page_list), &(cache->part_cache));
  }

  list_add_tail(obj_list, &(page_to_slab(page)->free_list));
}

/*
 * Destory a slab cache.
 * Never use "cache" any more after calling this function
 */
void slab_destory_cache(struct kcache* cache)
{
  struct list_head * cur = NULL;
  struct page * cur_page = NULL;

  if (!cache){
    DEBUG("NULL cache\n\r");
    return ;
  }

  list_for_each(cur, &(cache->full_cache)){
    cur_page = container_of(cur, struct page, page_list);
    pmm_free_one(cur_page);
  }

  list_for_each(cur, &(cache->part_cache)){
    cur_page = container_of(cur, struct page, page_list);
    pmm_free_one(cur_page);
  }

  list_del(&(cache->kc_list_entry));
  slab_free_obj(cache);
}

/*
 * Show all userable caches now.
 */
void slab_show_all_kcache()
{
  struct list_head * cur_list;
  struct kcache * cache;

  list_for_each(cur_list, &kcache_list){
    cache = container_of(cur_list, struct kcache, kc_list_entry);
    printk("%10s size = %d\n\r", cache->name, cache->obj_size);
  }
}
