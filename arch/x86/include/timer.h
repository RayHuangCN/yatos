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

#ifndef __ARCH_TIMER_H
#define __ARCH_TIMER_H

#include <arch/system.h>

#define TIMER_IRQ_NUM 0x20

void arch_timer_init(unsigned long hz);

#endif /* __ARCH_TIMER_H */
