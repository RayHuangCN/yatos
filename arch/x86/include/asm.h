/*
 *  Pio
 *
 *  Copyright (C) 2017 ese@ccnt.zju
 *
 *  ---------------------------------------------------
 *  Started at 2017/3/30 by Ray
 *
 *  ---------------------------------------------------
 *
 *  This file is subject to the terms and conditions of the GNU General Public
 *  License.
 */

#ifndef __ARCH_ASM_H
#define __ARCH_ASM_H

#define system_hlt() asm("hlt")
extern unsigned  int pio_in8(unsigned int address);
extern void pio_out8(unsigned int value, unsigned int address);
extern void pio_out16(unsigned int value, unsigned int address);
extern unsigned int  pio_in16(unsigned int address);
extern void putc(char  c);

#endif /* __ARCH_ASM_H */
