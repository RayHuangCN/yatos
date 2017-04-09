/*************************************************
 *   Author: Ray Huang
 *   Date  : 2017/4/9
 *   Email : rayhuang@126.com
 *   Desc  : VFS
 ************************************************/
#include <arch/system.h>
#include <yatos/printk.h>
#include <yatos/vfs.h>
#include <yatos/list.h>
#include <yatos/mm.h>
/******** filesystem init functions ****************/
typedef void (*vf_init_fun)();

extern void ext2_init();

vf_init_fun vf_inits[] = {
  ext2_init,
  NULL
};

/***************************************************/



#define VFS_FILE_NAME_LEN 128
#define VFS_FTYPE_NAME_LEN 32

static struct list_head inode_list;
static struct list_head dentry_list;
static struct list_head file_list;
static struct list_head vf_type_list;

static struct inode vfs_root;
static struct vfs_filesystem_type * root_vf_type;

static struct kcache * inode_cache;
static struct kcache * dentry_cache;
static struct kcache * sb_cache;
static struct kcache * vf_type_cache;
static struct kcache * file_cache;


static struct vfs_filesystem_type * vfs_find_ftype(const char *name)
{
  struct list_head *cur = NULL;
  struct vfs_filesystem_type *cur_vf = NULL;
  list_for_each(cur, &vf_type_list){
    cur_vfs = container_of(cur, struct vfs_filesystem_type, vfs_ftype_list);
    if (!strncmp(name, cur_vfs->name , VFS_FTYPE_NAME_LEN))
        return cur_vfs;
  }
  return NULL;
}


static void vfs_init_lists()
{
  INIT_LIST_HEAD(&node_list);
  INIT_LIST_HEAD(&dentry_list);
  INIT_LIST_HEAD(&file_list);
  INIT_LIST_HEAD(*vf_type_list);

}
static void vfs_init_caches()
{
  inode_cache = slab_create_cache( sizeof(struct vfs_inode), vfs_inode_constr ,vfs_inode_distr, "inode_cache");


}

static void vfs_init_vf_types()
{
  vf_init_fun vf_init = vf_inits;
  while (vf_init)
    vf_init();

}
static void vfs_init_root()
{

}

void vfs_init()
{
  vfs_init_lists();
  vfs_init_caches();
  vfs_init_vf_types();
  vfs_init_root();
}
