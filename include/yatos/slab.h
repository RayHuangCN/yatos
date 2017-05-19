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

#ifndef __YATOS_SLAB_H
#define __YATOS_SLAB_H

#include <arch/system.h>
#include <yatos/mm.h>
#include <yatos/list.h>

#define page_to_slab(page) (&(page->use_for.slab_frame))
#define objs_per_page(kcache) (PAGE_SIZE / kcache->obj_size)

typedef void (*cache_constr_fun)(void *);
typedef void (*cache_distr_fun)(void *);

struct kcache
{
  unsigned long obj_size;
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

#endif /* __YATOS_SLAB_H */
