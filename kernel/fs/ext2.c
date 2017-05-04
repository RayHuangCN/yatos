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
#include <yatos/bitmap.h>
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
static struct bitmap ** block_maps;
static struct bitmap ** inode_maps;

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

static int ext2_alloc_inode()
{
  int i;
  int ret_num;
  for (i = 0; i < group_num; i++){
    if (!inode_maps[i]){
      inode_maps[i] = bitmap_create(block_size * 8);
      if (!inode_maps[i]){
        printk("can not create inode map\n");
        return -1;
      }
      read_block(fs_group_infos[i].bg_inode_bitmap, (char *)inode_maps[i]->map);
    }
    ret_num = bitmap_alloc(inode_maps[i]);
    if (ret_num >= 0){
      write_block(fs_group_infos[i].bg_inode_bitmap, (char *)inode_maps[i]->map);
      break;
    }
  }
  if (i == group_num)
    return -1;

  return ret_num + (i * fs_sb->s_inodes_per_group) + 1;
}

static void ext2_clean_block(uint32 block_num)
{
  read_block(block_num, com_buffer);
  memset(com_buffer, 0, block_size);
  write_block(block_num, com_buffer);
}

static int ext2_alloc_block()
{
  int i;
  int ret_num;
  for (i = 0; i < group_num; i++){
    if (!block_maps[i]){
      block_maps[i] = bitmap_create(block_size * 8);
      if (!block_maps[i]){
        printk("can not create block map\n");
        return -1;
      }
      read_block(fs_group_infos[i].bg_block_bitmap, (char *)block_maps[i]->map);
    }
    ret_num = bitmap_alloc(block_maps[i]);
    if (ret_num >= 0){
      write_block(fs_group_infos[i].bg_block_bitmap, (char *)block_maps[i]->map);
      break;
    }
  }
  if (i == group_num)
    return -1;
  ret_num = ret_num + (i * fs_sb->s_blocks_per_group) + 1;
  ext2_clean_block(ret_num);
  return ret_num;

}

static void ext2_free_inode(int inode_num)
{
  int g_num = (inode_num - 1) / fs_sb->s_inodes_per_group;
  int g_offset = (inode_num - 1) % fs_sb->s_inodes_per_group;
  if (g_num >= group_num)
    return ;
  if (!inode_maps[g_num]){
    inode_maps[g_num] = bitmap_create(block_size * 8);
    if (!inode_maps[g_num]){
      printk("can not create inode map\n");
      return ;
    }
    read_block(fs_group_infos[g_num].bg_inode_bitmap, (char *)inode_maps[g_num]->map);
  }

  bitmap_free(inode_maps[g_num], g_offset);
  write_block(fs_group_infos[g_num].bg_inode_bitmap, (char *)inode_maps[g_num]->map);
}

static void ext2_free_block(int block_num)
{
  int g_num = (block_num - 1) / fs_sb->s_blocks_per_group;
  int g_offset = (block_num - 1) / fs_sb->s_blocks_per_group;
  if (g_num >= group_num)
    return ;
  if (!block_maps[g_num]){
    block_maps[g_num] = bitmap_create(block_size * 8);
    if (!block_maps[g_num]){
      printk("can not create block map\n");
      return ;
    }
    read_block(fs_group_infos[g_num].bg_block_bitmap, (char *)block_maps[g_num]->map);
  }
  bitmap_free(block_maps[g_num], g_offset);
  write_block(fs_group_infos[g_num].bg_block_bitmap, (char *)block_maps[g_num]->map);
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
  block_maps = (struct bitmap **)mm_kmalloc(sizeof(struct bitmap *) * group_num);
  memset(block_maps, 0 , sizeof(struct bitmap *) * group_num);
  inode_maps = (struct bitmap **)mm_kmalloc(sizeof(struct bitmap *) * group_num);
  memset(block_maps, 0, sizeof(struct bitmap *) * group_num);
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
  struct ext2_inode * einode = (struct ext2_inode *)inode->inode_data;
  uint32 file_size = einode->i_size;
  int i;
  uint32 block_offset;
  list_for_each(cur, &(inode->data_buffers)){
    cur_buffer = container_of(cur, struct fs_data_buffer, list_entry);
    if (!BUFFER_IS_DIRTY(cur_buffer))
        continue;
    block_offset = cur_buffer->block_offset * (FS_DATA_BUFFER_SIZE / block_size);
    for (i = 0; i < FS_DATA_BUFFER_SIZE / block_size && block_offset * block_size < file_size; i++, block_offset++)
      write_block(cur_buffer->block_num[i], cur_buffer->buffer + i * block_size);
    BUFFER_CLER_DIRTY(cur_buffer);
  }
  ext2_write_inode(inode->inode_num, inode->inode_data);
}

int ext2_find_file(const char *name, struct fs_inode * parent)
{
  struct ext2_inode * einode = (struct ext2_inode*)parent->inode_data;
  uint32 file_size = einode->i_size;
  uint32 buffer_num = 0;
  uint32 cur_offset = 0;
  char *buffer, *cur_data;
  struct ext2_dir_entry_2 * cur_entry;

  while (cur_offset < file_size){
    buffer = fs_inode_get_buffer(parent, buffer_num)->buffer;
    cur_data = buffer;

    cur_entry = (struct ext2_dir_entry_2 *)cur_data;
    while (cur_offset < file_size && cur_data < buffer + FS_DATA_BUFFER_SIZE){
      //find
      if (strlen(name) == cur_entry->name_len &&
          strncmp(name, cur_entry->name, cur_entry->name_len) == 0)
        return cur_entry->inode;
      //not find
      cur_offset += cur_entry->rec_len;
      cur_data += cur_entry->rec_len;
      cur_entry = (struct ext2_dir_entry_2 *)cur_data;
    }
    buffer_num++;
  }
  return -1;
}


void ext2_release_inode(struct ext2_inode* inode)
{
  slab_free_obj(inode);
}

void ext2_init_root(struct fs_inode* root)
{
  root->inode_data = root_inode;
  root->inode_num = 2;
  root->mode = root_inode->i_mode;
}

int ext2_fill_buffer(struct fs_inode* inode,struct fs_data_buffer* new_buf)
{

  struct ext2_inode * einode = inode->inode_data;
  uint32 block_offset = new_buf->block_offset * (FS_DATA_BUFFER_SIZE / block_size);
  uint32 block_num;
  uint32 * indirect_buffer = (uint32*)com_buffer;
  uint32 size = einode->i_size;
  int i;
  uint32 tmp_num;
  for (i = 0; i < FS_DATA_BUFFER_SIZE / block_size && block_offset * block_size < size; i++, block_offset++){
    if (block_offset < LEVE0_TOTAL){

      block_num = einode->i_block[block_offset];
      if (!block_num)
        block_num = einode->i_block[block_offset] = ext2_alloc_block();

    }else if (block_offset < LEVE1_TOTAL){

      if (!einode->i_block[12])
        einode->i_block[12] = ext2_alloc_block();

      read_block(einode->i_block[12], (char*)indirect_buffer);
      block_num = indirect_buffer[block_offset - 12];

      if (!block_num){
        block_num = indirect_buffer[block_offset - 12] = ext2_alloc_block();
        write_block(einode->i_block[12], (char*)indirect_buffer);
      }

    }else if (block_offset < LEVE2_TOTAL){
      if (!einode->i_block[13])
        einode->i_block[13] = ext2_alloc_block();

      read_block(einode->i_block[13], (char*)indirect_buffer);
      block_num = indirect_buffer[(block_offset - 12) / LEVE1_BLOCKS];
      if (!block_num){
        block_num = indirect_buffer[(block_offset - 12) / LEVE1_BLOCKS] = ext2_alloc_block();
        write_block(einode->i_block[13], (char*)indirect_buffer);
      }
      read_block(block_num, (char*)indirect_buffer);
      tmp_num = block_num;
      block_num = indirect_buffer[(block_offset -12) % LEVE1_BLOCKS];
      if (!block_num){
        block_num = indirect_buffer[(block_offset - 12) % LEVE1_BLOCKS];
        write_block(tmp_num, (char*)indirect_buffer);
      }
    }else{

      if (!einode->i_block[14])
        einode->i_block[14] = ext2_alloc_block();
      read_block(einode->i_block[14] , (char*)indirect_buffer);
      block_num = indirect_buffer[(block_offset - 12) / LEVE2_BLOCKS];
      if (!block_num){
        block_num = indirect_buffer[(block_offset - 12) / LEVE2_BLOCKS] = ext2_alloc_block();
        write_block(einode->i_block[14], (char*)indirect_buffer);
      }

      read_block(block_num, (char*)indirect_buffer);
      tmp_num = block_num;
      block_num = indirect_buffer[((block_offset - 12) % LEVE2_BLOCKS) / LEVE1_BLOCKS];
      if (!block_num){
        block_num = indirect_buffer[((block_offset - 12) % LEVE2_BLOCKS) / LEVE1_BLOCKS] = ext2_alloc_block();
        write_block(tmp_num, (char*)indirect_buffer);
      }

      read_block(block_num, (char*)indirect_buffer);
      tmp_num = block_num;
      block_num = indirect_buffer[((block_offset - 12) % LEVE2_BLOCKS) % LEVE1_BLOCKS];
      if (!block_num){
        block_num = indirect_buffer[((block_offset - 12) % LEVE2_BLOCKS) % LEVE1_BLOCKS] = ext2_alloc_block();
        write_block(tmp_num, (char*)indirect_buffer);
      }
    }
    read_block(block_num, new_buf->buffer + i * block_size);
    new_buf->block_num[i] = block_num;
  }
  ext2_write_inode(inode->inode_num, einode);
  return 0;
}



uint32 ext2_get_block_size()
{
  return block_size;
}

int ext2_fill_inode(struct fs_inode *inode, int inode_num)
{
  struct ext2_inode * einode = ext2_get_inode_by_num(inode_num);
  if (!einode)
    return 1;
  inode->inode_data = einode;
  inode->inode_num = inode_num;
  inode->mode = einode->i_mode;
  return 0;
}

static struct ext2_dir_entry_2 *  ext2_alloc_dir_entry(const char* name,struct fs_inode* dir)
{
  struct ext2_inode * einode = (struct ext2_inode*)dir->inode_data;
  uint32 file_size = einode->i_size;
  uint32 buffer_num = 0;
  uint32 cur_offset = 0;
  char *buffer, *cur_data;
  uint32 name_len = strlen(name);
  uint32 target_space = name_len + sizeof(struct ext2_dir_entry_2) + 1;
  struct ext2_dir_entry_2 * cur_entry = NULL;
  struct ext2_dir_entry_2 * new_entry = NULL;
  struct fs_data_buffer * data_buffer = NULL;
  unsigned long base;
  while (cur_offset < file_size){
    data_buffer = fs_inode_get_buffer(dir, buffer_num);
    buffer = data_buffer->buffer;
    if (!data_buffer || !buffer)
      break;
    cur_data = buffer;
    cur_entry = (struct ext2_dir_entry_2 *)cur_data;
    //find a useable space
    while (cur_offset < file_size && cur_data < buffer + FS_DATA_BUFFER_SIZE){
      base = (unsigned long)(cur_data + sizeof(*cur_entry) + cur_entry->name_len);
      base = (base + 7) / 8 * 8;

      if (cur_data + cur_entry->rec_len >= (char *)base + target_space){
        new_entry = (struct ext2_dir_entry_2*)base;
        new_entry->rec_len = cur_data + cur_entry->rec_len - (char*)base;
        cur_entry->rec_len = (char*)base - cur_data;
        BUFFER_SET_DIRTY(data_buffer);
        break;
      }
      cur_offset += cur_entry->rec_len;
      cur_data += cur_entry->rec_len;
      cur_entry = (struct ext2_dir_entry_2 *)cur_data;
    }
    if (new_entry)
      break;
    buffer_num++;
  }
  //no useable space, dir file size should be incred
  if (!new_entry){
    einode->i_size += block_size;
    data_buffer = fs_inode_get_buffer(dir, (einode->i_size - block_size) / FS_DATA_BUFFER_SIZE);
    buffer = data_buffer->buffer;
    if (!data_buffer || !buffer)
      return NULL;
    BUFFER_SET_DIRTY(data_buffer);
    buffer += (einode->i_size % FS_DATA_BUFFER_SIZE);
    new_entry = (struct ext2_dir_entry_2 *)buffer;
    new_entry->rec_len = block_size;
  }
  new_entry->name_len = name_len;
  strncpy(new_entry->name, name, name_len);
  return new_entry;
}

int ext2_create_file(const char* name,struct fs_inode* parent, uint16 mode)
{
  struct ext2_dir_entry_2 * entry = ext2_alloc_dir_entry(name,parent);
  if (!entry)
    return -1;
  int new_inode_num = ext2_alloc_inode();
  if (new_inode_num < 0)
    return -1;
  entry->inode = new_inode_num;
  entry->file_type = 1;// REG_FILE
  struct ext2_inode *inode = ext2_get_inode_by_num(new_inode_num);
  if (!inode)
    return -1;
  memcpy(inode, parent->inode_data, sizeof(*inode));

  inode->i_mode = mode;
  int i;
  for (i = 0; i < 12; i++)
    inode->i_block[i] = 0;
  inode->i_block[12] = 0;
  inode->i_block[13] = 0;
  inode->i_block[14] = 0;
  inode->i_links_count = 1;
  inode->i_size = 0;
  inode->i_blocks = 0;

  inode->i_mode = EXT2_IFREG | EXT2_IRUSR | EXT2_IWUSR;

  ext2_sync_data(parent);
  ext2_write_inode(new_inode_num, inode);
  return new_inode_num;
}

int ext2_readdir(struct fs_file *file, struct kdirent * ret)
{
  struct ext2_dir_entry_2 tmp_dirent;
  int n = fs_read(file, (char*)&tmp_dirent, sizeof(tmp_dirent));
  if (n < sizeof(tmp_dirent) || tmp_dirent.inode == 0)
    return -1;

  char name[tmp_dirent.name_len + 1];
  n = fs_read(file, name, tmp_dirent.name_len);
  if (n < tmp_dirent.name_len)
    return -1;
  name[tmp_dirent.name_len] = '\0';

  //to next dirent
  fs_seek(file,tmp_dirent.rec_len - sizeof(tmp_dirent) - tmp_dirent.name_len, SEEK_CUR);
  ret->inode_num = tmp_dirent.inode;
  strcpy(ret->name, name);
  return 0;
}
