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


/* open/fcntl.  */
#define O_ACCMODE	   0003
#define O_RDONLY	     00
#define O_WRONLY	     01
#define O_RDWR		     02
#ifndef O_CREAT
# define O_CREAT	   0100	/* Not fcntl.  */
#endif
#ifndef O_EXCL
# define O_EXCL		   0200	/* Not fcntl.  */
#endif
#ifndef O_NOCTTY
# define O_NOCTTY	   0400	/* Not fcntl.  */
#endif
#ifndef O_TRUNC
# define O_TRUNC	  01000	/* Not fcntl.  */
#endif
#ifndef O_APPEND
# define O_APPEND	  02000
#endif
#ifndef O_NONBLOCK
# define O_NONBLOCK	  04000
#endif
#ifndef O_NDELAY
# define O_NDELAY	O_NONBLOCK
#endif
#ifndef O_SYNC
# define O_SYNC	       04010000
#endif
#define O_FSYNC		O_SYNC
#ifndef O_ASYNC
# define O_ASYNC	 020000
#endif
#ifndef __O_LARGEFILE
# define __O_LARGEFILE	0100000
#endif

#ifndef __O_DIRECTORY
# define __O_DIRECTORY  0200000
#endif
#ifndef __O_NOFOLLOW
# define __O_NOFOLLOW	0400000
#endif
#ifndef __O_CLOEXEC
# define __O_CLOEXEC   02000000
#endif
#ifndef __O_DIRECT
# define __O_DIRECT	 040000
#endif
#ifndef __O_NOATIME
# define __O_NOATIME   01000000
#endif
#ifndef __O_PATH
# define __O_PATH     010000000
#endif
#ifndef __O_DSYNC
# define __O_DSYNC	 010000
#endif
#ifndef __O_TMPFILE
# define __O_TMPFILE   (020000000 | __O_DIRECTORY)
#endif

#define O_CLOEXEC __O_CLOEXEC

#define	__S_IFMT	0170000	/* These bits determine file type.  */
#define	__S_IFDIR	0040000	/* Directory.  */
#define	__S_IFCHR	0020000	/* Character device.  */
#define	__S_IFBLK	0060000	/* Block device.  */
#define	__S_IFREG	0100000	/* Regular file.  */
#define	__S_IFIFO	0010000	/* FIFO.  */
#define	__S_IFLNK	0120000	/* Symbolic link.  */
#define	__S_IFSOCK	0140000	/* Socket.  */


#define	__S_ISUID	04000	/* Set user ID on execution.  */
#define	__S_ISGID	02000	/* Set group ID on execution.  */
#define	__S_ISVTX	01000	/* Save swapped text after use (sticky).  */
#define	__S_IREAD	0400	/* Read by owner.  */
#define	__S_IWRITE	0200	/* Write by owner.  */
#define	__S_IEXEC	0100	/* Execute by owner.  */




#define S_IFMT		__S_IFMT
#define S_IFDIR	__S_IFDIR
#define S_IFCHR	__S_IFCHR
#define S_IFBLK	__S_IFBLK
#define S_IFREG	__S_IFREG
#define S_IFIFO	__S_IFIFO
#define S_IFLNK	__S_IFLNK
#define S_IFSOCK __S_IFSOCK

#define	__S_ISTYPE(mode, mask)	(((mode) & __S_IFMT) == (mask))

#define	S_ISDIR(mode)	 __S_ISTYPE((mode), __S_IFDIR)
#define	S_ISCHR(mode)	 __S_ISTYPE((mode), __S_IFCHR)
#define	S_ISBLK(mode)	 __S_ISTYPE((mode), __S_IFBLK)
#define	S_ISREG(mode)	 __S_ISTYPE((mode), __S_IFREG)
#define S_ISFIFO(mode)	 __S_ISTYPE((mode), __S_IFIFO)

#define S_ISLNK(mode)	 __S_ISTYPE((mode), __S_IFLNK)
#define S_ISSOCK(mode) __S_ISTYPE((mode), __S_IFSOCK)


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
	int (*ioctl)(struct fs_file * file, int requst, unsigned long arg);
	int (*readdir)(struct fs_file * file, struct kdirent * ret);
};

struct fs_inode
{
	uint32 inode_num;
	void *inode_data;
	mode_t mode;
	struct list_head data_buffers;
	struct fs_data_buffer * recent_data;
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
struct fs_file * fs_get_root();
struct fs_file * fs_new_file();
struct fs_inode * fs_new_inode();

struct fs_data_buffer * fs_inode_get_buffer(struct fs_inode *inode, unsigned long buffer_offset);

#endif
