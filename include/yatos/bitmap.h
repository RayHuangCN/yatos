#ifndef __YATOS_BITMAP_H
#define __YATOS_BITMAP_H

/*************************************************
 *   Author: Ray Huang
 *   Date  : 2017/4/13
 *   Email : rayhuang@126.com
 *   Desc  : bitmap manage
 ************************************************/
#include <arch/system.h>


struct bitmap
{
  uint32 count;
  uint8 map[];
};

struct bitmap * bitmap_create(uint32 count);
void bitmap_destory(struct bitmap * bm);

struct bitmap * bitmap_clone(struct bitmap *  from);
void bitmap_copy(struct bitmap * des, struct bitmap *  src);

int bitmap_alloc(struct bitmap *  bm);
void bitmap_free(struct bitmap * bi, int num);
int bitmap_count(struct bitmap * bm);

void bitmap_set(struct bitmap * bm, int num);
void bitmap_clr(struct bitmap * bm, int num);
int bitmap_check(struct bitmap * bm, int num);

#endif
