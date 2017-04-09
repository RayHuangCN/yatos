/*************************************************
 *   Author: Ray Huang
 *   Date  : 2017/4/9
 *   Email : rayhuang@126.com
 *   Desc  : disk
 ************************************************/
#include <arch/system.h>
#include <arch/asm.h>
#include <arch/disk.h>

static void wait_for_busy()
{
  while ((pio_in8(0x1f7) & 0x80));
}


static void wait_for_data()
{
  wait_for_busy();
  while (!(pio_in8(0x1f7) & 0x08));
}

void disk_read(uint32 sector_number,uint32 sector_count ,uint16* buffer)
{
  wait_for_busy();
  pio_out8(sector_count & 0xff, 0x1f2);
  pio_out8(sector_number & 0xff , 0x1f3);
  pio_out8((sector_number >> 8) & 0xff, 0x1f4);
  pio_out8((sector_number >> 16) & 0xff, 0x1f5);
  pio_out8((sector_number >> 24) | 0xe0, 0x1f6);
  pio_out8(0x20, 0x1f7);
  wait_for_data();
  uint32 i;
  for (i = 0; i < sector_count * 256; i++)
    buffer[i] = pio_in16(0x1f0);
}

void disk_write(uint32 sector_number,uint32 sector_count,uint16* buffer)
{
  wait_for_busy();
  pio_out8(sector_count & 0xff, 0x1f2);
  pio_out8(sector_number & 0xff , 0x1f3);
  pio_out8((sector_number >> 8) & 0xff, 0x1f4);
  pio_out8((sector_number >> 16) & 0xff, 0x1f5);
  pio_out8((sector_number >> 24) | 0xe0, 0x1f6);
  pio_out8(0x30, 0x1f7);
  wait_for_data();
  uint32 i;
  for (i = 0; i < sector_count * 256; i++){
    wait_for_data();
    pio_out16(buffer[i], 0x1f0);
  }
}
