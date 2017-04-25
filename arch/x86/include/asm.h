#ifndef __ASM_H
#define __ASM_H

/*************************************************
*   Author: Ray Huang
*   Date  : 2017/3/30
*   Email : rayhuang@126.com
*   Desc  : the declear of asm function
************************************************/

#define system_hlt() asm("hlt")

extern unsigned  int pio_in8(unsigned int address);
extern void pio_out8(unsigned int value, unsigned int address);
extern void pio_out16(unsigned int value, unsigned int address);
extern unsigned int  pio_in16(unsigned int address);

extern void putc(char  c);


#endif
