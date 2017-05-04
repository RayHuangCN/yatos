/*************************************************
 *   Author: Ray Huang
 *   Date  : 2017/3/30
 *   Email : rayhuang@126.com
 *   Desc  : operation of vga screen
 ************************************************/
#include <arch/asm.h>
#include <arch/system.h>
#include <arch/vga.h>
#include <printk/string.h>

void vga_set_cursor(int x,int y)
{
  unsigned short cursor = x + y * VGA_COL_NUM;
  pio_out8(0x0e, 0x3d4);
  pio_out8((cursor >> 8) &  0xff, 0x3d5);

  pio_out8(0x0f, 0x3d4);
  pio_out8(cursor & 0xff, 0x3d5);
}

void vga_set_base(char * base)
{
  memcpy((char *)VGA_VMM_START, base, PAGE_SIZE);
}

void vga_putc(char c, char attr, int x,int y)
{
  char * base = (char *)VGA_VMM_START;
  int offset = (x + y * VGA_COL_NUM) * 2;
  base[offset] = c;
  base[offset + 1] = attr;
}
