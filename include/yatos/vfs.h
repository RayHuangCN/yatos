#ifndef __YATOS_VFS_H
#define __YATOS_VFS_H

/*************************************************
 *   Author: Ray Huang
 *   Date  : 2017/4/9
 *   Email : rayhuang@126.com
 *   Desc  : VFS interface
 ************************************************/
#include <arch/system.h>
#include <yatos/printk.h>

struct vfs_supper_block
{
  unsigned long count;
};

struct vfs_inode
{
  unsigned long count;
};


struct vfs_dentry
{
  unsigned long count;
  char name[VFS_FILE_NAME_LEN];
  struct vfs_inode * inode;
  struct vfs_dentry * parent;
  struct list_head  childs;
  struct list_head bro_list;
  void * private;
};

struct vfs_file
{
  unsigned long count;
  unsigned long off_t;



};

struct vfs_filesystem_opr
{

};

struct vfs_filesystem_type
{
  char name[32];
  struct list_head vfs_ftype_list;
  unsigned long count;
  struct vfs_filesystem_opr * vf_opr;
};


void vfs_init();

int vfs_register(const char * name, struct vfs_filesystem_opr * vf_opr);
void vfs_unregister(const char *name);





#endif
