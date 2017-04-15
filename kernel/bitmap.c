/*************************************************
 *   Author: Ray Huang
 *   Date  : 2017/4/13
 *   Email : rayhuang@126.com
 *   Desc  : bitmap
 ************************************************/
#include <yatos/bitmap.h>
#include <yatos/mm.h>
#include <yatos/tools.h>



struct bitmap * bitmap_create(uint32 count)
{
  uint32 allc_size = (count + 31) / 32 * 4;
  struct bitmap  * bm = mm_kmalloc(allc_size + 4);
  if (!bm)
    return NULL;

  memset(bm, 0, allc_size);
  bm->count = count;
  return bm->map;
}


void bitmap_destory(struct bitmap * bm)
{
  mm_kfree(bm);
}


struct bitmap *  bitmap_clone(struct bitmap * from)
{
  uint32 alloc_size = (from->count + 31) / 32 * 4 + 4;
  struct bitmap * new = mm_kmalloc(alloc_size);
  memcpy(new, bm, alloc_size);
  return new;
}


void bitmap_copy(struct bitmap * des, struct bitmap * src)
{
  if (des->count != src->count){
    printk("bitmap_copy unequ count!\n");
    return ;
  }
  uint32 alloc_size = (des->count + 31) / 32 * 4 + 4;
  memcpy(des, src, alloc_size);
}

int bitmap_count(struct bitmap * bm)
{
  return bm->count;
}

int bitmap_alloc(struct bitmap * bi)
{
  uint32 count = bi->count;
  int i, j;
  for (i = 0; i < (count + 31) / 32; i++){
    if (bi->map[i] != 0xffffffff){
      for (j = 0 ; j < 32; j++)
        if (!((bi->map[i] >> j) & 1U) && (i * 32 + j < count)){
          bi->map[i] |= 1U << j;
          return i * 32 + j;
        }
    }
  }
  return -1;
}


void bitmap_free(struct bitmap * bi, int num)
{
  if (num >= bi->count)
    return ;
  bi->map[num / 32] &=  ~(1U << (num % 32));
}
