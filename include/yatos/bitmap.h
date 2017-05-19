/*
 *  Bitmap interface
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

#ifndef __YATOS_BITMAP_H
#define __YATOS_BITMAP_H

#include <arch/system.h>
#include <yatos/mm.h>
#include <yatos/tools.h>

#define bitmap_count(bm) (bm->count)
#define bitmap_destory(bm) mm_kfree(bm);
#define bitmap_free(bm, num) (bm->map[num / 8] &=  ~(1U << (num % 8)))
#define bitmap_set(bm, num) (bm->map[num / 8] |= 1 << (num % 8))
#define bitmap_clr(bm, num) (bm->map[num / 8] &= ~(1U << (num % 8)))
#define bitmap_check(bm, num)  (bm->map[num  / 8] & (1 << (num % 8)))
#define bitmap_copy(des, src) \
  do{\
    assert(des->count == src->count);\
    memcpy(des, src, (des->count + 7) / 8 + sizeof(struct bitmap));\
  }while (0)


struct bitmap
{
  uint32 count;
  uint8 map[];
};

struct bitmap * bitmap_create(uint32 count);
struct bitmap * bitmap_clone(struct bitmap *  from);
int bitmap_alloc(struct bitmap *  bm);

#endif /* __YATOS_BITMAP_H */
