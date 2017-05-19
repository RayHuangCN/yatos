/*
 *  VGA lowleve operations
 *
 *  Copyright (C) 2017 ese@ccnt.zju
 *
 *  ---------------------------------------------------
 *  Started at 2017/3/31 by Ray
 *
 *  ---------------------------------------------------
 *
 *  This file is subject to the terms and conditions of the GNU General Public
 *  License.
 */

#ifndef __ARCH_VGA_H
#define __ARCH_VGA_H

#include <arch/asm.h>

#define read_keyboard() pio_in8(0x60)
#define VGA_COL_NUM 80
#define VGA_ROW_NUM 25
#define VGA_CHAR_SIZE 2

void vga_set_cursor(int x,  int y);
void vga_set_base(char *base);
void vga_putc(char c, char attr, int x,int y);

#endif /* __ARCH_VGA_H */
