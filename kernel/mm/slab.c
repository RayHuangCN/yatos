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



}

void slab_init()
{
  slab_kcache_constr(&kcache_cache);
  kcache_cache.constr = slab_create_cache;
  kcache_cache.obj_size = sizeof(struct kcache);
  kcache_cache.obj_per_page = PAGE_SIZE / kcache_cache.obj_size;
}

struct kcache * slab_create_cache(unsigned long size,cache_constr_fun constr,cache_distr_fun distr)
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

  list_add_tail(&(ret_kc->kc_list_entry), &kcache_list);
  return ret_kc;
}
