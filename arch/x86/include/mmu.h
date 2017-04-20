#ifndef __ARCH_MMU_H
#define __ARCH_MMU_H

/*************************************************
 *   Author: Ray Huang
 *   Date  : 2017/4/19
 *   Email : rayhuang@126.com
 *   Desc  : mm low level
 ************************************************/
#include <arch/system.h>
#include <arch/regs.h>

#define PDT_MAX_NUM (PAGE_SIZE / 4)
#define PET_MAX_NUM (PAGE_SIZE / 4)
#define USER_SPACE_PDT_MAX_NUM (256 * 3)


#define get_pdt_entry(pdt_table_addr, target_addr) \
  (((uint32 *)pdt_table_addr)[target_addr >> 22])

#define set_pdt_entry(pdt_table_addr,target_addr, new_pdt)  \
  (((uint32 *)pdt_table_addr)[target_addr >> 22] = new_pdt)

#define get_pet_entry(pet_table_addr, target_addr) \
  (((uint32 *)pet_table_addr)[target_addr << 10 >> 22])

#define set_pet_entry(pet_table_addr, target_addr, new_pet) \
  (((uint32 *)pet_table_addr)[target_addr << 10 >> 22] = new_pet)

#define make_pdt(pet_table_addr, rw)\
  (pet_table_addr | (rw << 1) | 0x5)

#define make_pet(page_addr, rw) \
  (page_addr | (rw << 1) | 0x5)

#define get_pet_addr(pdt_e) \
  (pdt_e & ~(0xfff))

#define get_page_addr(pet_e)\
  (pet_e & ~(0xfff))

#define pet_writable(pet_e) \
  (pet_e & 0x2)

#define clr_writable(pet_e)\
  (pet_e &= ~0x2)

int mmu_map(unsigned long pdt, unsigned long vaddr, unsigned long paddr, unsigned long rw);
void mmu_init();
void mmu_flush();
uint32 mmu_page_fault_addr();
void mmu_set_page_table(uint32 addr);
#endif
