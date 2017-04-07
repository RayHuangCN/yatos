#ifndef __YATOS_SLAB_H
#define __YATOS_SLAB_H

/*************************************************
 *   Author: Ray Huang
 *   Date  : 2017/4/5
 *   Email : rayhuang@126.com
 *   Desc  : slab interface
 ************************************************/
#include <arch/system.h>
#include <yatos/mm.h>
#include <yatos/list.h>


#define page_to_slab(page) (&(page->use_for.slab_frame))

typedef void (*cache_constr_fun)(void *);
typedef void (*cache_distr_fun)(void *);

struct kcache
{
  unsigned long obj_size;
  unsigned long obj_per_page;
  struct list_head full_cache;
  struct list_head part_cache;
  struct list_head kc_list_entry;
  cache_constr_fun constr;
  cache_distr_fun distr;

  char name[32];
};
struct kcache * slab_create_cache(unsigned long size,
                                    cache_constr_fun constr,
                                  cache_distr_fun distr, const char * name);

void slab_destory_cache(struct kcache * cache);

void *slab_alloc_obj(struct kcache * cache);

void slab_free_obj(void * obj);

void slab_init();

void slab_show_all_kcache();





#endif
