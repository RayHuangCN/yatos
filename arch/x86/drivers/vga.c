/*************************************************
 *   Author: Ray Huang
 *   Date  : 2017/3/30
 *   Email : rayhuang@126.com
 *   Desc  : operation of vga screen
 ************************************************/
#include <arch/asm.h>
#include <arch/system.h>
#include <arch/vga.h>
/********* g_function ***************************/

void vga_clean(void)
{
  char *buffer = (char *)VGA_VMM_START;
  unsigned int i;
  for (i = 0; i < 2000; i++) {
    buffer[i << 1] = ' ';
    buffer[ (i << 1) + 1] = 0x7;
  }
}

unsigned short vga_get_cursor(void)
{
  unsigned short ans = 0;
  pio_out8(0x0e, 0x3d4);
  ans = pio_in8(0x3d5);

  ans <<= 8;

  pio_out8(0x0f, 0x3d4);
  ans |= pio_in8(0x3d5);
  return ans;
}

void vga_set_cursor(unsigned short cursor)
{
  pio_out8(0x0e, 0x3d4);
  pio_out8(cursor >> 8, 0x3d5);

  pio_out8(0x0f, 0x3d4);
  pio_out8(cursor & 0xff, 0x3d5);
}
void putc(char c)
{



  unsigned short cursor = vga_get_cursor();

  char * buffer = (char *)VGA_VMM_START;
  if (c == '\n')
    cursor += 80;
  else if (c == '\r')
    cursor = cursor / 80 * 80;
  else{

    buffer[cursor << 1] = c;
    buffer[(cursor << 1) + 1] = 0x07;
    cursor ++;
  }
  if (cursor >= 2000){

    unsigned short temp = cursor;

    for (cursor = 0; cursor < 2000 - 80; cursor++){
      buffer[cursor << 1] = buffer[(cursor + 80) << 1];
      buffer[(cursor << 1) + 1] = buffer[((cursor + 80) << 1) + 1];
    }
    for (;cursor < 2000; cursor++){
      buffer[cursor << 1] = ' ';
      buffer[ (cursor << 1) + 1] = 0x7;
    }

    cursor = temp - 80;
  }

  vga_set_cursor(cursor);
}
