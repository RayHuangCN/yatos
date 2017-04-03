/*************************************************
 *   Author: Ray Huang
 *   Date  : 2017/4/2
 *   Email : rayhuang@126.com
 *   Desc  : lowleve timer setup
 ************************************************/

/********* header files *************************/
#include <arch/asm.h>
#include <arch/system.h>
/********* g_define *****************************/
/********* g_variable ***************************/
/********* g_function ***************************/
//init 8253
void arch_timer_init(unsigned long hz)
{
  unsigned long count = 1193180  / hz ;
  pio_write(0x36 ,0x43);
  pio_write(count & 0xff, 0x40);
  pio_write(count >> 8, 0x40);
}

void arch_timer_ack()
{

}
