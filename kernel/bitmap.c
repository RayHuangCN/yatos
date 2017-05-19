/*
 *  Bitmap
 *
 *  Copyright (C) 2017 ese@ccnt.zju
 *
 *  ---------------------------------------------------
 *  Started at 2017/4/13 by Ray
 *
 *  ---------------------------------------------------
 *
 *  This file is subject to the terms and conditions of the GNU General Public
 *  License.
 */
#include <yatos/bitmap.h>
#include <yatos/mm.h>
#include <yatos/tools.h>
#include <yatos/printk.h>

/*
 * Create a new bitmap with "count" bits.
 */
struct bitmap * bitmap_create(uint32 count)
{
  uint32 allc_size = (count + 7) / 8 + 4;
  struct bitmap  * bm = mm_kmalloc(allc_size);

  if (!bm){
    DEBUG("can not create bitmap!");
    return NULL;
  }

  memset(bm, 0, allc_size);
  bm->count = count;
  return bm;
}

/*
 * Clone a new bitmap from "from".
 */
struct bitmap *  bitmap_clone(struct bitmap * from)
{
  unsigned long alloc_size = (from->count + 7) / 8 + sizeof(struct bitmap);
  struct bitmap * new = bitmap_create(from->count);

  if (!new){
    DEBUG("can not get new bitmap!");
    return NULL;
  }
  memcpy(new, from, alloc_size);
  return new;
}

/*
 * Alloc a free position from "bi".
 * If there is no free position, return -1.
 */
int bitmap_alloc(struct bitmap * bi)
{
  uint32 count = bi->count;
  int i, j;

  for (i = 0; i < (count + 7) / 8; i++){
    if (bi->map[i] != 0xff){
      for (j = 0 ; j < 8; j++)
        if (!((bi->map[i] >> j) & 1U) && (i * 8 + j < count)){
          bi->map[i] |= 1U << j;
          return i * 8 + j;
        }
    }
  }
  return -1;
}
