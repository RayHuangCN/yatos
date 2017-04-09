/*************************************************
 *   Author: Ray Huang
 *   Date  : 2017/4/5
 *   Email : rayhuang@126.com
 *   Desc  : slab memory manager
 ************************************************/
#include <yatos/printk.h>
#include <yatos/mm.h>
#include <yatos/slab.h>
#include <yatos/list.h>
#include <printk/string.h>


static struct kcache kcache_cache;
static struct list_head kcache_list;

void slab_kcache_constr(void * kcache)
{

  struct kcache * kc = (struct kcache *)kcache;
  INIT_LIST_HEAD(&(kc->full_cache));
  INIT_LIST_HEAD(&(kc->part_cache));

  kc->constr = NULL;
  kc->distr = NULL;
  kc->obj_size = 0;
  kc->obj_per_page = 0;
  kc->kc_list_entry.prev = NULL;
  kc->kc_list_entry.next = NULL;

  strcpy(kc->name, "kcache");
}

void slab_init()
{
  INIT_LIST_HEAD(&kcache_list);
  slab_kcache_constr(&kcache_cache);
  kcache_cache.constr = slab_kcache_constr;
  kcache_cache.obj_size = sizeof(struct kcache);
  kcache_cache.obj_per_page = PAGE_SIZE / kcache_cache.obj_size;
}

struct kcache * slab_create_cache(unsigned long size,cache_constr_fun constr,cache_distr_fun distr,
                                  const char * name)
{
  struct kcache * ret_kc;

  if (size < 16)
    size = 16;

  ret_kc = slab_alloc_obj(&kcache_cache);
  if (!ret_kc){
    printk("can not alloc kcache\n\r");
    return NULL;
  }

  ret_kc->constr = constr;
  ret_kc->distr = distr;
  ret_kc->obj_size = size;

  ret_kc->obj_per_page = PAGE_SIZE / ret_kc->obj_size;

  strncpy(ret_kc->name, name, 32);

  list_add_tail(&(ret_kc->kc_list_entry), &kcache_list);
  return ret_kc;
}


static struct page * slab_get_new_page(struct kcache * cache)
{
  struct page * new_page = pmm_alloc_one();
  int i;
  unsigned long page_addr;

  if (!new_page)
    return NULL;

  new_page->type = PMM_PAGE_TYPE_SLAB;

  page_to_slab(new_page)->parent = cache;
  INIT_LIST_HEAD(&(page_to_slab(new_page)->free_list));

  page_addr = paddr_to_vaddr(pmm_page_to_paddr(new_page));
  for (i = 0;i < PAGE_SIZE / cache->obj_size; i++){
    struct list_head * cur = (struct list_head *)page_addr;
    list_add_tail(cur, &(page_to_slab(new_page)->free_list));
    page_addr += cache->obj_size;
  }
  list_add_tail(&(new_page->page_list), &(cache->part_cache));
  return new_page;
}


void * slab_alloc_obj(struct kcache* cache)
{
  if (list_empty(&(cache->part_cache))){
    struct page * new_page = slab_get_new_page(cache);
    if (!new_page){
      printk("get new page error\n\r");
      return NULL;
    }
  }
  struct list_head * ret_page_list = cache->part_cache.next;
  struct page * ret_page = container_of(ret_page_list, struct page, page_list);
  struct list_head * ret_obj = page_to_slab(ret_page)->free_list.next;
  list_del(ret_obj);
  if (list_empty(&(page_to_slab(ret_page)->free_list))){
    list_del(ret_page_list);
    list_add_tail(ret_page_list, &(cache->full_cache));
  }

  if (cache->constr)
    cache->constr((void *)ret_obj);


  return (void *)ret_obj;
}


void slab_free_obj(void* obj)
{

  unsigned long addr = (unsigned long)obj;
  struct list_head * obj_list = (struct list_head*)addr;
  struct page *page = vaddr_to_page(addr >> PAGE_SHIFT << PAGE_SHIFT);
  struct kcache * cache = page_to_slab(page)->parent;
  if (list_empty(&(page_to_slab(page)->free_list))){
    list_del(&(page->page_list));
    list_add_tail(&(page->page_list), &(cache->part_cache));
  }

  list_add_tail(obj_list, &(page_to_slab(page)->free_list));

  if (cache->distr)
    cache->distr(obj);
}


void slab_destory_cache(struct kcache* cache)
{
  struct list_head * cur = NULL;
  struct page * cur_page = NULL;

  if (!cache){
    printk("NULL cache\n\r");
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


void slab_show_all_kcache()
{
  struct list_head * cur_list = NULL;
  struct kcache * cache = NULL;
  list_for_each(cur_list, &kcache_list){
    cache = container_of(cur_list, struct kcache, kc_list_entry);
    printk("%10s size = %d\n\r", cache->name, cache->obj_size);
  }
}
