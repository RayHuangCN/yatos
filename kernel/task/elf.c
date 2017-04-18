/*************************************************
 *   Author: Ray Huang
 *   Date  : 2017/4/12
 *   Email : rayhuang@126.com
 *   Desc  : elf loader
 ************************************************/
#include <yatos/elf.h>
#include <yatos/list.h>
#include <yatos/task.h>
#include <arch/system.h>
#include <yatos/fs.h>
#include <yatos/mm.h>

struct exec_bin * elf_parse(struct fs_file* file)
{
  //read head
  Elf32_Ehdr * elf_head = mm_kmalloc(sizeof(Elf32_Ehdr));
  struct exec_bin * bin = task_new_exec_bin();
  if (!elf_head || !bin)
    return NULL;

  uint32 read_n;
  fs_seek(file, 0 , SEEK_SET);
  read_n = fs_read(file, (uint8*)elf_head, sizeof(*elf_head));
  if (read_n != sizeof(*elf_head)){
    printk("can not read elf_head\n");
    return NULL;
  }

  bin->entry_addr = elf_head->e_entry;
  bin->exec_file = file;


  //search all sections
  uint32 i;
  uint32 secs_size = sizeof(Elf32_Shdr) * elf_head->e_shnum;
  Elf32_Shdr * secs = mm_kmalloc(secs_size);
  if (!secs)
    return NULL;

  fs_seek(file, elf_head->e_shoff, SEEK_SET);
  read_n = fs_read(file, (uint8*)secs, secs_size);
  if (read_n != secs_size){
    printk("can not read elf_sections\n");
    return NULL;
  }

  for (i = 0 ; i < elf_head->e_shnum; i++){
    if (secs[i].sh_flags & SHF_ALLOC){

      struct section * new_sec = task_new_section();
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
