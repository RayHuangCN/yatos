/*************************************************
 *   Author: Ray Huang
 *   Date  : 2017/4/19
 *   Email : rayhuang@126.com
 *   Desc  : mm low level
 ************************************************/
#include <arch/system.h>
#include <arch/regs.h>
#include <arch/mmu.h>
#include <yatos/mm.h>
void mmu_init()
{
  uint32 * pdt = (uint32*)INIT_PDT_TABLE_START;
  int i;
  for (i = 0; i < KERNEL_VMM_START / (4 * 10240 * 1024); i++)
    pdt[i] = 0;

}

int mmu_map(unsigned long pdt, unsigned long vaddr,unsigned long paddr, unsigned long rw)
{
  vaddr = PAGE_ALIGN(vaddr);
  paddr = PAGE_ALIGN(paddr);

  uint32 pdt_e, pet_e, pet_table_vaddr;
  pdt_e = get_pdt_entry(pdt, vaddr);
  if (!pdt_e){
    uint32 new_pet_table = (uint32)mm_kmalloc(PAGE_SIZE);
    if (!new_pet_table)
      return 1;
    memset((void *)new_pet_table, 0, PAGE_SIZE);
    new_pet_table = vaddr_to_paddr(new_pet_table);
    pdt_e = make_pdt(new_pet_table, 1);
    set_pdt_entry(pdt, vaddr, pdt_e);
  }


  pet_e = make_pet(paddr, rw);
  pet_table_vaddr = paddr_to_vaddr(get_pet_addr(pdt_e));
  set_pet_entry(pet_table_vaddr, vaddr,  pet_e);
  mmu_flush();
  return 0;
}
