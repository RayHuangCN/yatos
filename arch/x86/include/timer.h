#ifndef __ARCH_TIMER_H
#define __ARCH_TIMER_H

/*************************************************
 *   Author: Ray Huang
 *   Date  : 2017/4/2
 *   Email : rayhuang@126.com
 *   Desc  : timer lowleve interface
 ************************************************/

/********* header files *************************/
#include <arch/system.h>

/********* g_define *****************************/
#define TIMER_IRQ_NUM 0x20

/********* g_variable ***************************/
/********* g_function ***************************/
void arch_timer_init(unsigned long hz);



#endif
