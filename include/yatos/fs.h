/*
 *  Virtual File System interface
 *
 *  Copyright (C) 2017 ese@ccnt.zju
 *
 *  ---------------------------------------------------
 *  Started at 2017/4/9 by Ray
 *
 *  ---------------------------------------------------
 *
 *  This file is subject to the terms and conditions of the GNU General Public
 *  License.
 */

#ifndef __YATOS_FS_H
#define __YATOS_FS_H

#include <arch/system.h>
#include <yatos/list.h>
#include <yatos/printk.h>
#include <yatos/mm.h>

typedef int off_t;
typedef unsigned long mode_t;

#define O_ACCMODE	   0003
#define O_RDONLY	     00
#define O_WRONLY	     01
#define O_RDWR		     02

#define O_CREAT	   0100
#define O_EXCL		   0200
#define O_NOCTTY	   0400
#define O_TRUNC	  01000
#define O_APPEND	  02000
#define O_NONBLOCK	  04000
#define O_NDELAY	O_NONBLOCK
#define O_SYNC	       04010000
#define O_FSYNC		O_SYNC
#define O_ASYNC	 020000
#define O_CLOEXEC  02000000

#define F_DUPFD		0	/* dup */
#define F_GETFD		1	/* get close_on_exec */
#define F_SETFD		2	/* set/clear close_on_exec */
#define F_GETFL		3	/* get file->f_flags */
#define F_SETFL		4	/* set file->f_flags */
#define FD_CLOEXEC	1	/* actually anything with low bit set goes */

#define	S_IFMT	0170000	/* These bits determine file type.  */
#define	S_IFDIR	0040000	/* Directory.  */
#define	S_IFCHR	0020000	/* Character device.  */
#define	S_IFBLK	0060000	/* Block device.  */
#define	S_IFREG	0100000	/* Regular file.  */
#define	S_IFIFO	0010000	/* FIFO.  */
#define	S_IFLNK	0120000	/* Symbolic link.  */
#define	S_IFSOCK	0140000	/* Socket.  */

#define	S_ISUID	04000	/* Set user ID on execution.  */
#define	S_ISGID	02000	/* Set group ID on execution.  */
#define	S_ISVTX	01000	/* Save swapped text after use (sticky).  */
#define	S_IREAD	0400	/* Read by owner.  */
#define	S_IWRITE	0200	/* Write by owner.  */
#define	S_IEXEC	0100	/* Execute by owner.  */

#define	__S_ISTYPE(mode, mask)	(((mode) & S_IFMT) == (mask))
#define	S_ISDIR(mode)	 __S_ISTYPE((mode), S_IFDIR)
#define	S_ISCHR(mode)	 __S_ISTYPE((mode), S_IFCHR)
#define	S_ISBLK(mode)	 __S_ISTYPE((mode), S_IFBLK)
#define	S_ISREG(mode)	 __S_ISTYPE((mode), S_IFREG)
#define S_ISFIFO(mode)	 __S_ISTYPE((mode), S_IFIFO)
#define S_ISLNK(mode)	 __S_ISTYPE((mode), S_IFLNK)
#define S_ISSOCK(mode) __S_ISTYPE((mode), S_IFSOCK)

#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

#define MAX_PATH_LEN PAGE_SIZE
#define FS_DATA_BUFFER_SIZE PAGE_SIZE

struct fs_data_buffer
{
	unsigned long flag;
	unsigned long block_offset;
	unsigned long block_num[4];
	struct list_head list_entry;
	char * buffer;
};

struct kdirent
{
	int inode_num;
	int name_len;
	char name[256];
};

struct kstat
{
	int inode_num;
	mode_t mode;
	int links_count;
	off_t size;
};

#define BUFFER_DIRTY_MASK 0x1
#define BUFFER_IS_DIRTY(buff) (buff->flag & BUFFER_DIRTY_MASK)
#define BUFFER_SET_DIRTY(buff) (buff->flag |= BUFFER_DIRTY_MASK)
#define BUFFER_CLER_DIRTY(buff) (buff->flag &= ~BUFFER_DIRTY_MASK)


#define fs_get_file(file) (file->count++)
#define fs_put_file(file) \
	do{\
		file->count--;\
		if (!file->count)\
			slab_free_obj(file);\
	}while (0)

#define fs_get_inode(inode) (inode->count++);
#define fs_put_inode(inode) \
	do{\
		inode->count--;\
		if (!inode->count){ \
			if (inode->action && inode->action->sync)\
				inode->action->sync(inode);\
		}\
		if (!inode->count && inode->links_count <=0)\
			slab_free_obj(inode);\
	} while (0)

#define fs_close(file) fs_put_file(file)



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
	int (*ioctl)(struct fs_file * file, int requst, unsigned long arg);
	int (*readdir)(struct fs_file * file, struct kdirent * ret);
	int (*ftruncate)(struct fs_file * file, off_t length);
};

struct fs_inode
{
	uint32 inode_num;
	void *inode_data;
	mode_t mode;
	uint32 size;
	struct list_head data_buffers;
	struct fs_data_buffer * recent_data;
	unsigned long count;
    struct list_head list_entry;
	struct fs_inode_oper *action;
	struct fs_inode * parent;
	int links_count;
};

struct fs_file
{
	off_t cur_offset;
	unsigned long flag;
	unsigned long count;
	struct fs_inode * inode;
};

void fs_init();
struct fs_file * fs_open(const char * path, int flag, mode_t mode, int *ret);
struct fs_file * fs_open_stdin();
struct fs_file * fs_open_stdout();
struct fs_file * fs_open_stderr();
int fs_read(struct fs_file * file, char * buffer, unsigned long count);
int fs_write(struct fs_file * file, char * buffer, unsigned long count);
void fs_sync(struct fs_inode *file);
off_t fs_seek(struct fs_file * file, off_t offset, int whence);
struct fs_file * fs_new_file();
struct fs_inode * fs_new_inode();
struct fs_data_buffer * fs_inode_get_buffer(struct fs_inode *inode, unsigned long buffer_offset);

#endif /* __YATOS_FS_H */
