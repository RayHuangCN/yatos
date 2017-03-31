#ifndef __ASM_H
#define __ASM_H

/*************************************************
*   Author: Ray Huang
*   Date  : 2017/3/30
*   Email : rayhuang@126.com
*   Desc  : the declear of asm function
************************************************/

/********* g_function ***************************/
extern unsigned int pio_read(unsigned int address);
extern void pio_write(unsigned int value, unsigned int address);

extern void putc(char  c);


#endif
