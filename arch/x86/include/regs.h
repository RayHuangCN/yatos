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
struct pt_regs
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
  uint32 irq_num;
  uint32 erro_code;
  uint32 eip;
  uint32 cs;
  uint32 eflags;
  uint32 esp;
  uint32 ss;
};

struct tss
{
  uint32 pre_tss;
  uint32 esp0;
  uint32 ss0;
  uint32 esp1;
  uint32 ss1;
  uint32 esp2;
  uint32 ss2;
  uint32 cr3;
  uint32 eip;
  uint32 eflags;
  uint32 eax;
  uint32 ecx;
  uint32 edx;
  uint32 ebx;
  uint32 esp;
  uint32 ebp;
  uint32 esi;
  uint32 edi;
  uint32 es;
  uint32 cs;
  uint32 ss;
  uint32 ds;
  uint32 fs;
  uint32 gs;
  uint32 ldt;
  uint32 io;
};



/********* g_variable ***************************/
/********* g_function ***************************/




#endif
