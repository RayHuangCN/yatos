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


typedef int off_t;
typedef unsigned long mode_t;


#define O_RDONLY 1
#define O_WRONLY 2
#define O_RDWR (O_RDONLY | O_WRONLY)
#define O_CLOEXEC 4
#define O_CREAT 8
#define O_EXCL 16
#define O_TRUNC 32
#define O_APPEND 64
#define O_NONBLOCK 128
#define O_SYNC 256


#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

#define FS_BUFFER_SIZE PAGE_SIZE
#define MAX_PATH_LEN 4096
struct fs_data_buffer
{
	unsigned long flag;
	unsigned long block_offset;
	unsigned long ext2_block_num;
	struct list_head list_entry;
	char * buffer;
};

#define BUFFER_DIRTY_MASK 0x1
#define BUFFER_IS_DIRTY(buff) (buff->flag & BUFFER_DIRTY_MASK)
#define BUFFER_SET_DIRTY(buff) (buff->flag |= BUFFER_DIRTY_MASK)
#define BUFFER_CLER_DIRTY(buff) (buff->flag &= ~BUFFER_DIRTY_MASK)


struct fs_file;
struct fs_inode;
struct fs_inode_oper
{
	int (*read)(struct fs_file * file,  char * buffer, unsigned long count);
	int (*write)(struct fs_file * file,  char * buffer, unsigned long count);
	off_t (*seek)(struct fs_file * file, off_t offset, int whence);
	void (*sync)(struct fs_inode *inode);
	int  (*close)(struct fs_file * file);
	void (*release)(struct fs_inode *inode);
};

struct fs_inode
{
	uint32 inode_num;
	void *inode_data;
	struct list_head data_buffers;
	unsigned long count;
    struct list_head list_entry;
	struct fs_inode_oper *action;
};

struct fs_file
{
	off_t cur_offset;
	unsigned long flag;
	unsigned long count;
	struct fs_inode * inode;
};


void fs_init();
struct fs_file * fs_open(const char * path, int flag, mode_t mode);
struct fs_file * fs_open_stdin();
struct fs_file * fs_open_stdout();
struct fs_file * fs_open_stderr();

int fs_read(struct fs_file * file, char * buffer, unsigned long count);
int fs_write(struct fs_file * file, char * buffer, unsigned long count);
void fs_close(struct fs_file * file);
void fs_sync(struct fs_file *file);
off_t fs_seek(struct fs_file * file, off_t offset, int whence);
void fs_get_file(struct fs_file * file);
void fs_put_file(struct fs_file * file);
void fs_get_inode(struct fs_inode * inode);
void fs_put_inode(struct fs_inode  * inode);
struct fs_inode * fs_get_root();
struct fs_file * fs_new_file();
struct fs_inode * fs_new_inode();


#endif
