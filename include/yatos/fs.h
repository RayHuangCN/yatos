#ifndef __YATOS_FS_H
#define __YATOS_FS_H

/*************************************************
 *   Author: Ray Huang
 *   Date  : 20174/9
 *   Email : rayhuang@126.com
 *   Desc  : ext2 filesystem interface
 ************************************************/
#include <yatos/printk.h>
#include <yatos/list.h>
#include <arch/system.h>


#define FS_START_SECTOR 18432

struct fs_superblock
{
	uint32 inodes_count;			// Total # of inodes
  	uint32 blocks_count;			// Total # of blocks
  	uint32 r_blocks_count;		// # of reserved blocks for superuser
  	uint32 free_blocks_count;	
  	uint32 free_inodes_count;
	uint32 first_data_block;
	uint32 log_block_size;		// 1024 << Log2 block size  = block size
	uint32 log_frag_size;
	uint32 blocks_per_group;
	uint32 frags_per_group;
	uint32 inodes_per_group;
	uint32 mtime;					// Last mount time, in POSIX time
	uint32 wtime;					// Last write time, in POSIX time
	uint32 mnt_count;				// # of mounts since last check
	uint32 max_mnt_count;			// # of mounts before fsck must be done
	uint32 magic;					// 0xEF53
	uint32 state;
	uint32 errors;
	uint32 minor_rev_level;
	uint32 lastcheck;
	uint32 checkinterval;
	uint32 creator_os;
	uint32 rev_level;
	uint32 def_resuid;
	uint32 def_resgid;
} __attribute__((packed));


struct fs_block_group_desc
{
	uint32 block_bitmap;
	uint32 inode_bitmap;
	uint32 inode_table;
	uint16 free_blocks_count;
	uint16 free_inodes_count;
	uint16 used_dirs_count;
	uint16 pad[7];
} __attribute__((packed));


struct fs_inode
{
	uint16 mode;
	uint16 uid;
	uint32 size;
	uint32 atime;
	uint32 ctime;
	uint32 mtime;
	uint32 dtime;
	uint16 gid;
	uint16 links_count;
	uint32 blocks;
	uint32 flags;
	uint32 osdl;
	uint32 block[15];
	uint32 generation;
	uint32 file_acl;
	uint32 dri_acl;
	uint32 faddr;
	uint32 osd2[3];
} __attribute__((packed));

struct fs_dirent
{
	uint32 inode;
	uint16 rec_len;
	uint8 name_len;
	uint8 file_type;
	uint8 name[];
} __attribute__((packed));




void fs_init();


#endif
