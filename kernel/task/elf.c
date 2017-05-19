/*
 *  Elf parse
 *
 *  Copyright (C) 2017 ese@ccnt.zju
 *
 *  ---------------------------------------------------
 *  Started at 2017/4/12 by Ray
 *
 *  ---------------------------------------------------
 *
 *  This file is subject to the terms and conditions of the GNU General Public
 *  License.
 */

#include <yatos/elf.h>
#include <yatos/list.h>
#include <yatos/task.h>
#include <arch/system.h>
#include <yatos/fs.h>
#include <yatos/mm.h>

/*
 * Parse a elf file to be exec_bin.
 * Return exec_bin if successful or return NULL if any error.
 */
struct exec_bin * elf_parse(struct fs_file* file)
{
  //read head
  Elf32_Ehdr * elf_head = mm_kmalloc(sizeof(Elf32_Ehdr));
  struct exec_bin * bin = task_new_exec_bin();
  uint32 read_n, i;
  uint32 secs_size;
  Elf32_Shdr * secs;
  struct section * new_sec;

  if (!elf_head || !bin)
    return NULL;

  fs_seek(file, 0 , SEEK_SET);
  read_n = fs_read(file, (char *)elf_head, sizeof(*elf_head));
  if (read_n != sizeof(*elf_head)){
    printk("can not read elf_head\n");
    return NULL;
  }

  bin->entry_addr = elf_head->e_entry;
  bin->exec_file = file;

  //search all sections
  secs_size = sizeof(Elf32_Shdr) * elf_head->e_shnum;
  secs = mm_kmalloc(secs_size);
  if (!secs)
    return NULL;

  fs_seek(file, elf_head->e_shoff, SEEK_SET);
  read_n = fs_read(file, (char *)secs, secs_size);
  if (read_n != secs_size){
    printk("can not read elf_sections\n");
    return NULL;
  }

  for (i = 0 ; i < elf_head->e_shnum; i++){
    if (secs[i].sh_flags & SHF_ALLOC){

      new_sec = task_new_section();
      new_sec->file_offset = secs[i].sh_offset;
      new_sec->start_vaddr = secs[i].sh_addr;
      new_sec->len = secs[i].sh_size;

      if (secs[i].sh_type == SHT_NOBITS)
        new_sec->flag |= SECTION_NOBITS;
      if (secs[i].sh_flags & SHF_WRITE)
        new_sec->flag |= SECTION_WRITE;
      if (secs[i].sh_flags & SHF_EXECINSTR)
        new_sec->flag |= SECTION_EXEC;

      list_add_tail(&(new_sec->list_entry), &(bin->section_list));
    }
  }

  //free mm
  mm_kfree(elf_head);
  mm_kfree(secs);
  return bin;
}
