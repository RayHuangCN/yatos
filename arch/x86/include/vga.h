#ifndef __VGA_H
#define __VGA_H

/*************************************************
 *   Author: Ray Huang
 *   Date  : 2017/3/31
 *   Email : rayhuang@126.com
 *   Desc  : vga driver interface
 ************************************************/

/********* g_function ***************************/
unsigned short vga_get_cursor(void);
void vga_set_cursor(unsigned short);
void vga_clean(void);


#endif
