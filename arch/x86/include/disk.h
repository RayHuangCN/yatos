#ifndef __ARCH_DISK_H
#define __ARCH_DISK_H

/*************************************************
 *   Author: Ray Huang
 *   Date  : 2017/4/9
 *   Email : rayhuang@126.com
 *   Desc  : disk interface
 ************************************************/
#include <arch/system.h>
#include <arch/asm.h>



void disk_read(uint32 sector_number, uint32 sector_count, uint16 * buffer);
void disk_write(uint32 sector_number, uint32 sector_count, uint16 * buffer);
#endif
