/*
 *  Disk read and write
 *
 *  Copyright (C) 2017 ese@ccnt.zju
 *
 *  ---------------------------------------------------
 *  Started at 2017/4/9 by Ray
 *
 *  ---------------------------------------------------
 *
 *  This file is subject to the terms and conditions of the GNU General Public
 *  License.
 */

#ifndef __ARCH_DISK_H
#define __ARCH_DISK_H

#include <arch/system.h>
#include <arch/asm.h>

void disk_read(uint32 sector_number, uint32 sector_count, uint16 * buffer);
void disk_write(uint32 sector_number, uint32 sector_count, uint16 * buffer);

#endif /* __ARCH_DISK_H */
