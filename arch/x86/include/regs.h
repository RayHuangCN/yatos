#ifndef __REGS_H
#define __REGS_H

/*************************************************
 *   Author: Ray Huang
 *   Date  : 2017/3/31
 *   Email : rayhuang@126.com
 *   Desc  : regs define
 ************************************************/
/********* header files *************************/
#include <arch/system.h>

/********* g_define *****************************/
struct regs
{
  uint32 ebx;
  uint32 ecx;
  uint32 edx;
  uint32 esi;
  uint32 edi;
  uint32 ebp;
  uint32 eax;
  uint32 ds;
  uint32 es;
};

/********* g_variable ***************************/
/********* g_function ***************************/




#endif
