/*************************************************
 *   Author: Ray Huang
 *   Date  : 2017/4/9
 *   Email : rayhuang@126.com
 *   Desc  : ext2 filesystem
 ************************************************/
#include <arch/system.h>
#include <yatos/fs.h>
#include <yatos/list.h>
#include <yatos/mm.h>
#include <arch/disk.h>

static struct kcache * inode_cache;
static struct fs_superblock fs_sb;

static void fs_sb_init()
{
  disk_read(FS_START_SECTOR + 2, 2, &fs_sb);
  printk("free inodes = %u\n\r", fs_sb.free_inodes_count);
  printk("free block = %u\n\r", fs_sb.free_blocks_count);
  printk("last check = %x\n\r", fs_sb.lastcheck);
  printk("inodes count = %u\n\r", fs_sb.inodes_count);
  printk("blocks count = %u\n\r", fs_sb.blocks_count);
  printk("log block size = %u\n\r", fs_sb.log_block_size);
  printk("first data block = %u\n\r", fs_sb.first_data_block);
  printk("magic = %x\n\r", fs_sb.magic);
}

void fs_init()
{
  fs_sb_init();
}
