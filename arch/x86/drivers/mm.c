/*************************************************
 *   Author: Ray Huang
 *   Date  : 2017/4/19
 *   Email : rayhuang@126.com
 *   Desc  : mm low level
 ************************************************/
#include <arch/system.h>
#include <arch/regs.h>

void mm_arch_init()
{
  uint32 * pdt = (uint32*)INIT_PDT_TABLE_START;
  int i;
  for (i = 0; i < KERNEL_VMM_START / (4 * 10240 * 1024); i++)
    pdt[0] = 0;

}

void mm_arch_flush_mmu()
{
  asm("movl %cr3, %eax\n"
      "movl %eax, %cr3\n");
}
