/*************************************************
 *   Author: Ray Huang
 *   Date  : 2017/4/9
 *   Email : rayhuang@126.com
 *   Desc  : ext2 filesystem
 ************************************************/
#include <arch/system.h>
#include <yatos/ext2.h>
#include <yatos/list.h>
#include <yatos/mm.h>
#include <arch/disk.h>
#include <yatos/tools.h>
#include <printk/string.h>
#include <yatos/fs.h>

#define FS_START_SECTOR 18432
#define BLOCK_TO_SECTOR(block_num)  (FS_START_SECTOR +  (block_size / 512 * block_num))

#define FS_SEC_SIZE 512
#define fs_alloc_struct(type) (type*)mm_kmalloc(sizeof(type))
#define fs_free_struct(addr) mm_kree(addr)
#define INODE_NUM_PER_BLOCK (block_size / INODE_SIZE)

#define LEVE0_BLOCKS 12
#define LEVE0_TOTAL 12
#define LEVE1_BLOCKS (block_size / 4)
#define LEVE1_TOTAL (LEVE0_BLOCKS + LEVE1_BLOCKS)
#define LEVE2_BLOCKS ((block_size / 4) * (block_size / 4))
#define LEVE2_TOTAL (LEVE1_TOTAL + LEVE2_BLOCKS)
#define BLOCKS_PER_BLOCK (block_size / 4)

static uint32 block_size;
static uint32 sector_per_block;

static struct ext2_super_block  *fs_sb;
static struct ext2_group_desc  *fs_group_infos;
static uint32 group_num;
static struct ext2_inode       *root_inode;
static char *com_buffer;

static struct kcache * inode_cache;
static struct kcache * block_cache;
static struct kcache * dir_entry_cache;

static void read_block(uint32 block_num,  char  * buffer)
{
  uint32 sec = BLOCK_TO_SECTOR(block_num);
  disk_read(sec, sector_per_block, (uint16*)buffer);
}

static void write_block(uint32 block_num, char *buffer)
{
  uint32 sec = BLOCK_TO_SECTOR(block_num);
  disk_write(sec, sector_per_block, (uint16*)buffer);
}

static struct ext2_inode * ext2_get_inode_by_num(uint32 num)
{
  struct ext2_inode *ret = slab_alloc_obj(inode_cache);
  if (!ret){
    printk("can not alloc inode\n\r");
    return NULL;
  }

  struct ext2_group_desc * gdesc = fs_group_infos + (num - 1) / fs_sb->s_inodes_per_group;
  uint32 g_offset = (num - 1) % fs_sb->s_inodes_per_group;
  uint32 block = g_offset / INODE_NUM_PER_BLOCK + gdesc->bg_inode_table;

  read_block(block, com_buffer);

  uint32 b_offset = (g_offset % INODE_NUM_PER_BLOCK) * INODE_SIZE;
  memcpy(ret, com_buffer + b_offset , sizeof(*ret));
  return ret;
}

static void ext2_write_inode(uint32 num, struct ext2_inode * inode)
{
  struct ext2_group_desc * gdesc = fs_group_infos + (num - 1) / fs_sb->s_inodes_per_group;
  uint32 g_offset = (num - 1) % fs_sb->s_inodes_per_group;
  uint32 block = g_offset / INODE_NUM_PER_BLOCK + gdesc->bg_inode_table;

  read_block(block, com_buffer);

  uint32 b_offset = (g_offset % INODE_NUM_PER_BLOCK) * INODE_SIZE;
  memcpy(com_buffer + b_offset, inode, sizeof(*inode));

  write_block(block, com_buffer);
}


static void ext2_init_sb()
{
  fs_sb = fs_alloc_struct(struct ext2_super_block);
  if (!fs_sb)
    go_die("fs_init_sb can not alloc sb");

  disk_read(FS_START_SECTOR + 2, 2, (uint16*)com_buffer);

  memcpy(fs_sb, com_buffer, sizeof(*fs_sb));

  block_size = 1024 * (1 << fs_sb->s_log_block_size);
  sector_per_block = block_size / 512;
  group_num = (fs_sb->s_blocks_count - fs_sb->s_first_data_block - 1) / fs_sb->s_blocks_per_group + 1;
}


static void ext2_init_group()
{
  fs_group_infos = (struct ext2_group_desc*) mm_kmalloc(sizeof(*fs_group_infos) * group_num);
  if (!fs_group_infos)
    go_die("can not alloc fs_group_infos");

  read_block(2, com_buffer);
  memcpy(fs_group_infos, com_buffer, sizeof(*fs_group_infos) * group_num);
}




static void ext2_init_caches()
{


  inode_cache = slab_create_cache(sizeof(struct ext2_inode), NULL, NULL, "inode cache");
  if (!com_buffer || !inode_cache)
    go_die("fs_init_cache alloc mm error");

  block_cache = slab_create_cache(block_size, NULL, NULL,"block cache");
  if (!block_cache)
    go_die("fs_init_cache block cache error\n\r");

  dir_entry_cache = slab_create_cache(sizeof(struct ext2_dir_entry) , NULL ,NULL, "dir entry cache");
  if (!dir_entry_cache)
    go_die("fs_init_cache dir entry cache error\n\r");
}




void ext2_init()
{
  com_buffer = (char*)mm_kmalloc(4096);
  ext2_init_sb();
  ext2_init_caches();
  ext2_init_group();
  root_inode = ext2_get_inode_by_num(2);
}

void ext2_sync_data(struct fs_inode* inode)
{
  //update all block buffer;
  struct list_head *cur;
  struct fs_data_buffer * cur_buffer;

  list_for_each(cur, &(inode->data_buffers)){
    cur_buffer = container_of(cur, struct fs_data_buffer, list_entry);
    if (!BUFFER_IS_DIRTY(cur_buffer))
        continue;
    write_block(cur_buffer->ext2_block_num, cur_buffer->buffer);
    BUFFER_CLER_DIRTY(cur_buffer);
  }
  ext2_write_inode(inode->inode_num, inode->inode_data);
}

int ext2_find_file(const char *name, struct fs_inode * parent, struct fs_inode *ret)
{
  struct ext2_inode * inode  = parent->inode_data;
  uint32 file_size = inode->i_size;
  uint32 cur_blocks = 0;
  char * cur_iter;
  struct ext2_dir_entry_2 * cur_dentry;
  struct ext2_inode * ret_inode;
  uint32 inode_num;
  int len = strlen(name);
  while (file_size){
    //direct
    int i;
    for (i = 0; i < 12 && file_size; i++, cur_blocks++){
      read_block(inode->i_block[i], com_buffer);
      cur_iter = com_buffer;
      while (cur_iter < com_buffer + block_size){
        cur_dentry = (struct ext2_dir_entry_2*)cur_iter;
        if (len == cur_dentry->name_len && !strncmp(cur_dentry->name, name, cur_dentry->name_len)){
          inode_num = cur_dentry->inode;
          goto found;
        }
        cur_iter += cur_dentry->rec_len;
        file_size-= cur_dentry->rec_len;
      }
    }
    //1leve indirect
    //2leve indirect
    //3leve indirect
  }
  return 0;

found:
  ret_inode = ext2_get_inode_by_num(inode_num);
  if (!ret_inode)
    return 0;

  ret->inode_num = inode_num;
  ret->inode_data = ret_inode;
  return 1;
}


void ext2_free_inode(struct ext2_inode *inode)
{
  slab_free_obj(inode);
}

void ext2_init_root(struct fs_inode* root)
{
  root->inode_data = root_inode;
  root->inode_num = 2;
}


int ext2_fill_buffer(struct fs_inode* inode,struct fs_data_buffer* new_buf)
{
  struct ext2_inode * einode = inode->inode_data;
  uint32 block_offset = new_buf->block_offset;
  uint32 block_num;
  uint32 * indirect_buffer = (uint32*)com_buffer;
  if (block_offset < LEVE0_TOTAL){

    block_num = einode->i_block[block_offset];

  }else if (block_offset < LEVE1_TOTAL){

    read_block(einode->i_block[12], (char*)indirect_buffer);
    block_num = indirect_buffer[block_offset - 12];

  }else if (block_offset < LEVE2_TOTAL){

    read_block(einode->i_block[13], (char*)indirect_buffer);
    block_num = indirect_buffer[(block_offset - 12) / LEVE1_BLOCKS];
    read_block(block_num, (char*)indirect_buffer);
    block_num = indirect_buffer[(block_offset -12) % LEVE1_BLOCKS];

  }else{
    read_block(einode->i_block[14] , (char*)indirect_buffer);
    block_num = indirect_buffer[(block_offset - 12) / LEVE2_BLOCKS];
    read_block(block_num, (char*)indirect_buffer);
    block_num = indirect_buffer[((block_offset - 12) % LEVE2_BLOCKS) / LEVE1_BLOCKS];
    read_block(block_num, (char*)indirect_buffer);
    block_num = indirect_buffer[((block_offset - 12) % LEVE2_BLOCKS) % LEVE1_BLOCKS];
  }
  read_block(block_num, new_buf->buffer);
  new_buf->ext2_block_num = block_num;
  return 0;
}



uint32 ext2_get_block_size()
{
  return block_size;
}
