#ifndef __YATOS_FS_H
#define __YATOS_FS_H

/*************************************************
 *   Author: Ray Huang
 *   Date  : 2017/4/10
 *   Email : rayhuang@126.com
 *   Desc  : comon fs interfapce
 ************************************************/
#include <arch/system.h>
#include <yatos/list.h>
#include <yatos/printk.h>
#include <yatos/ext2.h>


struct fs_data_buffer
{
	unsigned long flag;
	unsigned long block_offset;
	unsigned long ext2_block_num;
	struct list_head list_entry;
	uint8 * buffer;
};

struct fs_inode
{
	unsigned long inode_num;
	struct ext2_inode * inode_data;
	struct list_head data_buffers;
	unsigned long count;
    struct list_head list_entry;
};



struct fs_file
{
	unsigned long cur_offset;
	unsigned long open_flag;
	unsigned long count;
	struct fs_inode * inode;
};




void fs_init();
struct fs_file * fs_open(const char * path, struct fs_inode * cur_dir);
int fs_read(struct fs_file * file, char * buffer, unsigned long count);
int fs_write(struct fs_file * file, char * buffer, unsigned long count);
void fs_close(struct fs_file * file);
void fs_sync(struct fs_file *file);








void fs_get_file(struct fs_file * file);
void fs_put_file(struct fs_file * file);

void fs_get_inode(struct fs_inode * inode);
void fs_put_inode(struct fs_inode  * inode);
extern struct fs_inode *root_dir;



















#endif
