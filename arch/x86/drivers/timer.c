/*
 *  Timer lowleve operations
 *
 *  Copyright (C) 2017 ese@ccnt.zju
 *
 *  ---------------------------------------------------
 *  Started at 2017/4/2 by Ray
 *
 *  ---------------------------------------------------
 *
 *  This file is subject to the terms and conditions of the GNU General Public
 *  License.
 */

#include <arch/asm.h>
#include <arch/system.h>

//init 8253
void arch_timer_init(unsigned long hz)
{
  unsigned long count = 1193180  / hz ;
  pio_out8(0x36 ,0x43);
  pio_out8(count & 0xff, 0x40);
  pio_out8(count >> 8, 0x40);
}

void arch_timer_ack()
{

}
