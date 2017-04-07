/*************************************************
 *   Author: Ray Huang
 *   Date  : 2017/4/5
 *   Email : rayhuang@126.com
 *   Desc  : slab memory manager
 ************************************************/
#include <yatos/mm.h>
#include <yatos/slab.h>

static kcache kcache_cache;


void slab_init()
{
}

struct kcache * slab_create_cache(unsigned long size,cache_constr_fun constr,cache_distr_fun distr)
{

}
