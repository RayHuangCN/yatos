#ifndef __VGA_H
#define __VGA_H

/*************************************************
 *   Author: Ray Huang
 *   Date  : 2017/3/31
 *   Email : rayhuang@126.com
 *   Desc  : vga driver interface
 ************************************************/
#include <arch/asm.h>

#define read_keyboard() pio_in8(0x60)

#define VGA_COL_NUM 80
#define VGA_ROW_NUM 25
#define VGA_CHAR_SIZE 2

void vga_set_cursor(int x,  int y);
void vga_set_base(char *base);
void vga_putc(char c, char attr, int x,int y);

#endif
