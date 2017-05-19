/*
 *  Ext2 filesystem operations
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

#include <arch/system.h>
#include <yatos/ext2.h>
#include <yatos/list.h>
#include <yatos/mm.h>
#include <arch/disk.h>
#include <yatos/tools.h>
#include <printk/string.h>
#include <yatos/fs.h>
#include <yatos/bitmap.h>
#include <yatos/errno.h>

#define BLOCK_TO_SECTOR(block_num)  (FS_START_SECTOR +  (block_size / 512 * block_num))

#define INODE_NUM_PER_BLOCK (block_size / INODE_SIZE)
#define LEVE0_BLOCKS 12
#define LEVE0_TOTAL 12
#define LEVE1_BLOCKS (block_size / 4)
#define LEVE1_TOTAL (LEVE0_BLOCKS + LEVE1_BLOCKS)
#define LEVE2_BLOCKS ((block_size / 4) * (block_size / 4))
#define LEVE2_TOTAL (LEVE1_TOTAL + LEVE2_BLOCKS)
#define LEVE3_BLOCKS ((block_size / 4) * (block_size / 4) * (block_size / 4))
#define LEVE3_TOTAL (LEVE2_TOTAL + LEVE3_BLOCKS)
#define BLOCKS_PER_BLOCK (block_size / 4)

static uint32 block_size;
static uint32 sector_per_block;
static uint32 group_num;

static struct ext2_super_block  *fs_sb;
static struct ext2_group_desc  *fs_group_infos;
static struct bitmap ** block_maps;
static struct bitmap ** inode_maps;
static struct ext2_inode *root_inode;

static struct kcache * inode_cache;
static struct kcache * block_cache;
static struct kcache * dir_entry_cache;

/*
 * Read and write block to disk according to block_num.
 */
static void read_block(uint32 block_num,  void  * buffer)
{
  uint32 sec = BLOCK_TO_SECTOR(block_num);
  disk_read(sec, sector_per_block, (uint16*)buffer);
}

static void write_block(uint32 block_num, void *buffer)
{
  uint32 sec = BLOCK_TO_SECTOR(block_num);
  disk_write(sec, sector_per_block, (uint16*)buffer);
}

/*
 * Get a ext2 inode from disk by inode number.
 * This functio will auto alloc ext2 inode struct, and fill it.
 * Return NULL if any error.
 */
static struct ext2_inode * ext2_get_inode_by_num(uint32 num)
{
  struct ext2_inode *ret;
  struct ext2_group_desc * gdesc;
  uint32 g_offset;
  uint32 block;
  uint32 b_offset;
  char * buffer;

  ret = slab_alloc_obj(inode_cache);
  if (!ret){
    DEBUG("can not alloc inode\n\r");
    return NULL;
  }

  buffer = mm_kmalloc(block_size);
  if (!buffer){
    DEBUG("can not alloc  buffer\n");
    return NULL;
  }

  gdesc = fs_group_infos + (num - 1) / fs_sb->s_inodes_per_group;
  g_offset = (num - 1) % fs_sb->s_inodes_per_group;
  block = g_offset / INODE_NUM_PER_BLOCK + gdesc->bg_inode_table;
  b_offset = (g_offset % INODE_NUM_PER_BLOCK) * INODE_SIZE;

  read_block(block, buffer);
  memcpy(ret, buffer + b_offset , sizeof(*ret));

  mm_kfree(buffer);
  return ret;
}

/*
 * Check a bitmap, initate this bitmap if nessary.
 * Return 0 if successly or return -1 if fail.
 *
 * Note: this function only be used to initate inode bitmap or block bitmap.
 */
static int ext2_check_bitmap(struct bitmap ** bm_pptr, int data_block_num)
{
  if (*bm_pptr)
    return 0;
  *bm_pptr = bitmap_create(block_size * 8);
  if (!*bm_pptr){
    DEBUG("can not create bitmap in ext2_check_bitmap!");
    return -1;
  }
  read_block(data_block_num, (*bm_pptr)->map);
  return 0;
}

/*
 * Alloc a inode according to inode maptable.
 * Return inode number if successful or return -1 if any error.
 */
static int ext2_alloc_inode()
{
  int i;
  int ret_num;

  //Find from all group
  for (i = 0; i < group_num; i++){
    if (ext2_check_bitmap(inode_maps + i, fs_group_infos[i].bg_inode_bitmap))
      return -1;
    ret_num = bitmap_alloc(inode_maps[i]);
    if (ret_num >= 0) break;
  }

  if (i == group_num)
    return -1;
  return ret_num + (i * fs_sb->s_inodes_per_group) + 1;
}

/*
 * Alloc a free block.
 * Return block number if successful or return -1 if any error.
 *
 * Note: new block will be initated to be all zero
 */
static int ext2_alloc_block()
{
  int i;
  int ret_num;
  char * buffer = mm_kmalloc(block_size);
  if (!buffer)
    return -1;

  for (i = 0; i < group_num; i++){
    if (ext2_check_bitmap(block_maps + i, fs_group_infos[i].bg_block_bitmap))
      return -1;
    ret_num = bitmap_alloc(block_maps[i]);
    if (ret_num >= 0) break;
  }
  if (i == group_num)
    return -1;
  ret_num = ret_num + (i * fs_sb->s_blocks_per_group) + 1;
  memset(buffer, 0, block_size);
  write_block(ret_num, buffer);
  return ret_num;
}

/*
 * Free a inode by inode number.
 * Return 0 if successful or return -1 if any error.
 */
int ext2_free_inode(int inode_num)
{
  int g_num = (inode_num - 1) / fs_sb->s_inodes_per_group;
  int g_offset = (inode_num - 1) % fs_sb->s_inodes_per_group;
  assert(g_num < group_num);

  if (ext2_check_bitmap(inode_maps + g_num, fs_group_infos[g_num].bg_inode_bitmap))
    return -1;
  bitmap_free(inode_maps[g_num], g_offset);
  return 0;
}

/*
 * Free a block by block number.
 * Return 0 if successful or return -1 if any error.
 */
static int ext2_free_block(int block_num)
{
  int g_num = (block_num - 1) / fs_sb->s_blocks_per_group;
  int g_offset = (block_num - 1) / fs_sb->s_blocks_per_group;
  assert(g_num < group_num);

  if (ext2_check_bitmap(block_maps + g_num, fs_group_infos[g_num].bg_block_bitmap))
    return -1;
  bitmap_free(block_maps[g_num], g_offset);
  return 0;
}

/*
 * Write a inode to disk.
 * Return 0 if successful or return -1 if any error.
 *
 * Note: the size of a inode is much small than a block, there are  more than one inode in one block,
 *       So, we need firstly read block to buffer, write inode to this buffer, and write block back to disk.
 */
static int ext2_write_inode(uint32 num, struct ext2_inode * inode)
{
  struct ext2_group_desc * gdesc = fs_group_infos + (num - 1) / fs_sb->s_inodes_per_group;
  uint32 g_offset = (num - 1) % fs_sb->s_inodes_per_group;
  uint32 block = g_offset / INODE_NUM_PER_BLOCK + gdesc->bg_inode_table;
  uint32 b_offset = (g_offset % INODE_NUM_PER_BLOCK) * INODE_SIZE;
  char * buffer = mm_kmalloc(block_size);

  if (!buffer)
    return -1;

  read_block(block, buffer);
  memcpy(buffer + b_offset, inode, sizeof(*inode));
  write_block(block, buffer);

  mm_kfree(buffer);
  return 0;
}

/*
 * Initate ext2 superblock .
 * System will go die if any error.
 */
static void ext2_init_sb()
{
  char * buffer;
  fs_sb = mm_kmalloc(sizeof(struct ext2_super_block));
  buffer = mm_kmalloc(PAGE_SIZE);

  assert(fs_sb);
  assert(buffer);

  //can not use read_block since read_block need block_size and block_size is uninitiate now
  disk_read(FS_START_SECTOR + 2, 2, (uint16*)buffer);
  memcpy(fs_sb, buffer, sizeof(*fs_sb));
  block_size = 1024 * (1 << fs_sb->s_log_block_size);
  sector_per_block = block_size / 512;
  group_num = (fs_sb->s_blocks_count - fs_sb->s_first_data_block - 1) / fs_sb->s_blocks_per_group + 1;
  mm_kfree(buffer);
}

/*
 * Initate ext2 groups.
 * System will go die if any error.
 */
static void ext2_init_group()
{
  char * buffer = mm_kmalloc(block_size);
  fs_group_infos = (struct ext2_group_desc*) mm_kmalloc(sizeof(*fs_group_infos) * group_num);
  assert(fs_group_infos);

  block_maps = (struct bitmap **)mm_kmalloc(sizeof(struct bitmap *) * group_num);
  memset(block_maps, 0 , sizeof(struct bitmap *) * group_num);

  inode_maps = (struct bitmap **)mm_kmalloc(sizeof(struct bitmap *) * group_num);
  memset(block_maps, 0, sizeof(struct bitmap *) * group_num);

  read_block(2, buffer);
  memcpy(fs_group_infos, buffer, sizeof(*fs_group_infos) * group_num);
  mm_kfree(buffer);
}

/*
 * Initate all slab caches of this file.
 * System will go die if any error.
 */
static void ext2_init_caches()
{
  inode_cache = slab_create_cache(sizeof(struct ext2_inode), NULL, NULL, "inode cache");
  assert(inode_cache);

  block_cache = slab_create_cache(block_size, NULL, NULL,"block cache");
  assert(block_cache);

  dir_entry_cache = slab_create_cache(sizeof(struct ext2_dir_entry) , NULL ,NULL, "dir entry cache");
  assert(dir_entry_cache);
}

/*
 * Initate ext2 filesystem operations.
 * System will go die if any error.
 */
void ext2_init()
{
  ext2_init_sb();
  ext2_init_caches();
  ext2_init_group();
  root_inode = ext2_get_inode_by_num(2);
}

/*
 * Sync data of a file. Inclues data blocks and ext2 inode.
 * No Return, No error.
 */
void ext2_sync_data(struct fs_inode* inode)
{
  //update all block buffer.
  struct list_head *cur;
  struct fs_data_buffer * cur_buffer;
  struct ext2_inode * einode = (struct ext2_inode *)inode->inode_data;
  uint32 file_size = einode->i_size = inode->size;
  int i;
  uint32 block_offset;

  assert(inode);

  list_for_each(cur, &(inode->data_buffers)){
    cur_buffer = container_of(cur, struct fs_data_buffer, list_entry);
    if (!BUFFER_IS_DIRTY(cur_buffer))
        continue;
    block_offset = cur_buffer->block_offset * (FS_DATA_BUFFER_SIZE / block_size);
    for (i = 0; i < FS_DATA_BUFFER_SIZE / block_size && block_offset * block_size < file_size; i++, block_offset++)
      write_block(cur_buffer->block_num[i], cur_buffer->buffer + i * block_size);
    BUFFER_CLER_DIRTY(cur_buffer);
  }

  //update ext2 inode.
  ext2_write_inode(inode->inode_num, inode->inode_data);
}

/*
 * Find a ext2 dir entry in a dir by name.
 * Return dir entry if successful or return NULL if not found.
 *
 * Note: if "write" is no-zero, the data_buffer which contains the target entry will be set to dirty.
 */
static struct ext2_dir_entry_2 *  ext2_find_entry(struct fs_inode * parent, const char *name, int write)
{
  uint32 file_size = parent->size;
  uint32 buffer_num = 0;
  uint32 cur_offset = 0;
  char *buffer, *cur_data;
  struct fs_data_buffer * data_buffer;
  struct ext2_dir_entry_2 * cur_entry;
  int i;

  assert(parent);
  assert(S_ISDIR(parent->mode));

  while (cur_offset < file_size){
    data_buffer = fs_inode_get_buffer(parent, buffer_num);
    buffer = data_buffer->buffer;

    //one data_buffer may contains more than one block data.
    for (i = 0; i < FS_DATA_BUFFER_SIZE / block_size; i++){
      cur_data = buffer + i * block_size;
      cur_entry = (struct ext2_dir_entry_2 *)cur_data;

      //foreach block in the data_buffer.
      while (cur_offset < file_size && cur_data < buffer + (i + 1) * block_size){
        //is this entry ?
        //if cur_entry->inode == 0, this is a invalid entry.
        if (cur_entry->inode &&
            strlen(name) == cur_entry->name_len &&
            strncmp(name, cur_entry->name, cur_entry->name_len) == 0){
          if (write)
            BUFFER_SET_DIRTY(data_buffer);
          return cur_entry;
        }
        //check next entry
        cur_offset += cur_entry->rec_len;
        cur_data += cur_entry->rec_len;
        cur_entry = (struct ext2_dir_entry_2 *)cur_data;
      }
    }
    //check next data_buffer.
    buffer_num++;
  }
  return NULL;
}

/*
 * Find a file in "dir" by "name".
 * Return file inode number if successful or return -1 if not found.
 */
int ext2_find_file(const char *name, struct fs_inode * dir)
{
  struct ext2_dir_entry_2 * ret = ext2_find_entry(dir, name, 0);
  return ret == NULL ? -1 : ret->inode;
}

/*
 * Fill a fs_data_buffer, block_offset is in the "new_buf".
 * Return 0 if successful or return -1 if any error;
 */
int ext2_fill_buffer(struct fs_inode* inode,struct fs_data_buffer* new_buf)
{
  struct ext2_inode * einode = inode->inode_data;
  uint32 block_offset = new_buf->block_offset * (FS_DATA_BUFFER_SIZE / block_size);
  uint32 block_num;
  uint32 * indirect_buffer = mm_kmalloc(block_size);
  uint32 size = inode->size;
  int i;
  uint32 tmp_num;

  assert(new_buf);
  assert(inode);

  if (!indirect_buffer)
    return -1;

  for (i = 0; i < FS_DATA_BUFFER_SIZE / block_size && block_offset * block_size < size; i++, block_offset++){
    if (block_offset < LEVE0_TOTAL){
      //12 direct blocks
      block_num = einode->i_block[block_offset];
      if (!block_num)
        block_num = einode->i_block[block_offset] = ext2_alloc_block();

    }else if (block_offset < LEVE1_TOTAL){
      //leve 1 indirect blocks
      if (!einode->i_block[12])
        einode->i_block[12] = ext2_alloc_block();

      read_block(einode->i_block[12], (char*)indirect_buffer);
      block_num = indirect_buffer[block_offset - 12];

      if (!block_num){
        block_num = indirect_buffer[block_offset - 12] = ext2_alloc_block();
        write_block(einode->i_block[12], (char*)indirect_buffer);
      }

    }else if (block_offset < LEVE2_TOTAL){
      //leve 2 indirect blocks
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
      //leve 3 indirect blocks
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
  return 0;
}


uint32 ext2_get_block_size()
{
  return block_size;
}

/*
 * Fill a inode by inode number.
 * Return 0 if successful or return -1 if any error.
 *
 * Note: file mode in ext2 filesysytem is the same as fs_inode mode.
 */
int ext2_fill_inode(struct fs_inode *inode, int inode_num)
{
  struct ext2_inode * einode = ext2_get_inode_by_num(inode_num);

  if (!einode)
    return -1;
  assert(inode);
  inode->inode_data = einode;
  inode->inode_num = inode_num;
  inode->mode = einode->i_mode;
  inode->size = einode->i_size;
  inode->links_count = einode->i_links_count;
  return 0;
}

/*
 * Alloc new dir entry from dir.
 * Return new dir entry if successful or return NULL if any error.
 *
 * Note: this function may increase the file size of dir.
 */
static struct ext2_dir_entry_2 *  ext2_alloc_dir_entry(const char* name,struct fs_inode* dir)
{
  uint32 file_size = dir->size;
  uint32 buffer_num = 0;
  uint32 cur_offset = 0;
  char *buffer, *cur_data;
  int name_len = strlen(name);
  int target_space = name_len + sizeof(struct ext2_dir_entry_2) + 1;
  struct ext2_dir_entry_2 * cur_entry = NULL;
  struct ext2_dir_entry_2 * new_entry = NULL;
  struct ext2_dir_entry_2 * pre_entry = NULL;
  struct fs_data_buffer * data_buffer = NULL;
  int base, cur_space = 0;
  int i;

  assert(name);
  assert(dir);
  assert(S_ISDIR(dir->mode));

  while (cur_offset < file_size){
    //get one buffer
    data_buffer = fs_inode_get_buffer(dir, buffer_num);
    buffer = data_buffer->buffer;
    if (!data_buffer || !buffer)
      break;

    //foreach block in the buffer
    for (i = 0; i < FS_DATA_BUFFER_SIZE / block_size; i++){
      cur_data = buffer + i * block_size;
      cur_entry = (struct ext2_dir_entry_2 *)cur_data;
      cur_space = 0;
      base = (int)cur_data;
      pre_entry = NULL;
      //find a useable space
      while (cur_offset < file_size && cur_data < buffer + (i + 1) * block_size){

        //free entry
        if (!cur_entry->inode)
          cur_space = (int)(cur_data + cur_entry->rec_len - base);
        //used entry
        else {
          pre_entry = cur_entry;
          base = (int)(cur_entry->name + cur_entry->name_len);
          //up align 8
          base = (base + 7) / 8 * 8;
          cur_space = (int)(cur_data + cur_entry->rec_len - base);
        }

        //if cur_space is ok?
        if (cur_space >= target_space){
          new_entry = (struct ext2_dir_entry_2*)base;
          new_entry->rec_len = cur_space;
          if (pre_entry)
            pre_entry->rec_len = base - (int)pre_entry;
          BUFFER_SET_DIRTY(data_buffer);
          break;
        }
        cur_offset += cur_entry->rec_len;
        cur_data += cur_entry->rec_len;
        cur_entry = (struct ext2_dir_entry_2 *)cur_data;
      }
      if (new_entry)
        break;
    }
    if (new_entry)
      break;
    buffer_num++;
  }
  //no useable space, increase dir file size.
  if (!new_entry){
    dir->size += block_size;
    data_buffer = fs_inode_get_buffer(dir, (dir->size - block_size) / FS_DATA_BUFFER_SIZE);
    buffer = data_buffer->buffer;
    if (!data_buffer || !buffer)
      return NULL;
    BUFFER_SET_DIRTY(data_buffer);
    buffer += (dir->size % FS_DATA_BUFFER_SIZE);
    new_entry = (struct ext2_dir_entry_2 *)buffer;
    new_entry->rec_len = block_size;
  }
  //initate dir entry that we alloc.
  new_entry->name_len = name_len;
  strncpy(new_entry->name, name, name_len);
  return new_entry;
}

/*
 * Create new file in "parent".
 * Return new file inode number if successful or return error code if any error.
 */
int ext2_create_file(const char* name,struct fs_inode* parent, uint16 mode)
{
  int new_inode_num;
  struct ext2_dir_entry_2 * entry;
  struct ext2_inode *inode;
  int i;

  assert(parent);
  assert(name);
  assert(S_ISDIR(parent->mode));

  new_inode_num = ext2_alloc_inode();
  if (new_inode_num < 0)
    return -ENOSPC;
  entry = ext2_alloc_dir_entry(name,parent);
  if (!entry){
    ext2_free_inode(new_inode_num);
    return -ENOSPC;
  }
  entry->inode = new_inode_num;
  entry->file_type = 1;// REG_FILE

  inode = ext2_get_inode_by_num(new_inode_num);
  if (!inode)
    return -ENOSPC;
  memcpy(inode, parent->inode_data, sizeof(*inode));
  for (i = 0; i < 12; i++)
    inode->i_block[i] = 0;
  inode->i_block[12] = 0;
  inode->i_block[13] = 0;
  inode->i_block[14] = 0;
  inode->i_links_count = 1;
  inode->i_size = 0;
  inode->i_blocks = 0;
  inode->i_mode = EXT2_IFREG | mode;

  ext2_write_inode(new_inode_num, inode);
  return new_inode_num;
}

/*
 * Read a dir entry from "file" according to the cur_offset of "file".
 * Return 0 and fill "ret" if successful or return error code if any error.
 */
int ext2_readdir(struct fs_file *file, struct kdirent * ret)
{
  struct ext2_dir_entry_2 tmp_dirent;
  int n;
  char name[MAX_FILE_NAME_LEN + 1];

  while (1){
    n = fs_read(file, (char*)&tmp_dirent, sizeof(tmp_dirent));
    if (n < sizeof(tmp_dirent))
      return -EINVAL;
    if (tmp_dirent.inode)
      break;
    else
      //skip this entry
      fs_seek(file, tmp_dirent.rec_len - sizeof(tmp_dirent), SEEK_CUR);
  }
  n = fs_read(file, name, tmp_dirent.name_len);
  if (n < tmp_dirent.name_len)
    return -EINVAL;
  name[tmp_dirent.name_len] = '\0';

  //to next dirent
  fs_seek(file,tmp_dirent.rec_len - sizeof(tmp_dirent) - tmp_dirent.name_len, SEEK_CUR);
  ret->inode_num = tmp_dirent.inode;
  strcpy(ret->name, name);
  return 0;
}

/*
 * Found the parent dir of a path.
 * "ret_dir" will be filled.
 */
static void ext2_path_dir(const char * path, char *ret_dir)
{
  strcpy(ret_dir, path);
  int len = strlen(ret_dir);
  char *cur = ret_dir + len - 1;

  while (cur >= ret_dir && *cur == '/')
    *cur-- = '\0';
  while (cur >= ret_dir && *cur != '/')
    cur--;
  *(cur + 1) = '\0';
}

/*
 * Found the last filename in a path.
 * "ret_name" will be filled.
 */
static void ext2_path_filename(const char * path, char * ret_name)
{
  int len = strlen(path);
  const char *cur = path + len - 1;
  char * name_cur;

  while (cur >= path && *cur == '/')
    cur--;
  while (cur >= path && *cur != '/')
    cur--;

  strcpy(ret_name, cur + 1);
  name_cur = ret_name;
  while (*name_cur && *cur != '/')
    name_cur++;
  *name_cur = '\0';
}

/*
 * Create a new dir in "parent".
 * Return 0 if successful and return error code if any error.
 */
static int ext2_create_dir(struct fs_inode * parent, const char * name, uint16 mode)
{
  int inode_num;
  int block_num;
  struct ext2_dir_entry_2 * entry;
  struct ext2_dir_entry_2 * cur_entry;
  struct ext2_inode * einode;
  int i;
  char * buffer;

  buffer = mm_kmalloc(block_size);
  if (!buffer)
    return -ENOMEM;

  //for new dir
  inode_num = ext2_alloc_inode();
  if (inode_num == -1){
    mm_kfree(buffer);
    return -ENOSPC;
  }

  //new dir first block data
  block_num = ext2_alloc_block();
  if (block_num == -1){
    mm_kfree(buffer);
    ext2_free_inode(inode_num);
    return -ENOSPC;
  }

  //new entry
  entry = ext2_alloc_dir_entry(name, parent);
  if (!entry){
    ext2_free_inode(inode_num);
    ext2_free_block(block_num);
    mm_kfree(buffer);
    return -ENOSPC;
  }

  //init new dir ext2 inode
  einode = ext2_get_inode_by_num(inode_num);
  entry->file_type = 0x4;
  entry->inode = inode_num;

  memcpy(einode,parent->inode_data, sizeof(struct ext2_inode));

  for (i = 0; i < 12; i++)
    einode->i_block[i] = 0;
  einode->i_block[12] = 0;
  einode->i_block[13] = 0;
  einode->i_block[14] = 0;
  einode->i_blocks = 2;
  einode->i_links_count = 2;
  einode->i_mode = mode | EXT2_IFDIR;
  einode->i_size = block_size; //1block default

  einode->i_block[0] = block_num;
  ext2_write_inode(inode_num, einode);

  //init . and ..
  cur_entry = (struct ext2_dir_entry_2*)buffer;
  cur_entry->name_len = 1;
  cur_entry->name[0] = '.';
  cur_entry->file_type = 0x4 ;
  cur_entry->inode = inode_num;
  cur_entry->rec_len = 16;

  cur_entry = (struct ext2_dir_entry_2 *)(buffer + 16);
  cur_entry->name_len = 2;
  cur_entry->name[0] = cur_entry->name[1] = '.';
  cur_entry->file_type = 0x4;
  cur_entry->inode = parent->inode_num;
  cur_entry->rec_len = block_size - 16;
  write_block(block_num, buffer);
  mm_kfree(buffer);
  return 0;
}

/*
 * Create new dir by "path".
 * Return 0 if successful or return error code if any error.
 */
int ext2_mkdir(const char * path, uint16 mode)
{
  char * p_path = (char*)mm_kmalloc(MAX_PATH_LEN + 1);
  char * name = (char *)mm_kmalloc(MAX_FILE_NAME_LEN + 1);
  int ret = -1;

  //open parent;
  struct fs_file * parent;
  ext2_path_dir(path, p_path);
  ext2_path_filename(path, name);

  if (!*name)
    return -EINVAL;
  if (!*p_path)
    parent = fs_open("./", O_RDWR, 0, &ret);
  else
    parent = fs_open(p_path, O_RDWR, 0,&ret);
  if (!parent)
    goto open_parent_error;
  if (!S_ISDIR(parent->inode->mode)){
    ret = -ENOTDIR;
    goto open_parent_error;
  }

  //file exist ?
  if (ext2_find_file(name, parent->inode) != -1){
    ret = -EEXIST;
    goto file_exsit;
  }
  ret = ext2_create_dir(parent->inode, name, mode);
  file_exsit:
 fs_close(parent);
 open_parent_error:
  mm_kfree(p_path);
  mm_kfree(name);
  return ret;
}

/*
 * Unlink a file by "path".
 * Return 0 if successful or return error code if any error.
 *
 * Note: dir is not allowed to unlink.
 */
int ext2_unlink(const char * path)
{
  struct fs_file * file;
  char * name;
  struct ext2_dir_entry_2 * entry;
  struct ext2_inode * einode;
  int ret = -1;

  file = fs_open(path, O_RDONLY, 0, &ret);
  if (!file)
    return ret;

  if (S_ISDIR(file->inode->mode)){
    ret = -EISDIR;
    goto is_dir;
  }

  name = (char *)mm_kmalloc(MAX_PATH_LEN + 1);
  if (!name){
    ret = -ENOMEM;
    goto no_mem;
  }

  ext2_path_filename(path, name);
  entry = ext2_find_entry(file->inode->parent, name, 1);
  entry->inode = 0;

  einode = file->inode->inode_data;
  einode->i_links_count--;
  file->inode->links_count--;
  ret = 0;
  mm_kfree(name);

 no_mem:
 is_dir:
  fs_close(file);
  return ret;
}

/*
 * Remove a dir by "path".
 * Return 0 if successful or return error code if any error.
 *
 * Note: it will return error if this dir is not empty.
 */
int ext2_rmdir(const char * path)
{
  struct fs_file * file;
  struct kdirent dirent;
  struct ext2_dir_entry_2 * entry;
  struct ext2_inode * einode;
  char name[MAX_FILE_NAME_LEN + 1];
  int ret = -1;

  ext2_path_filename(path, name);
  file = fs_open(path, O_RDONLY, 0, &ret);
  if (!file)
    return ret;

  if (!S_ISDIR(file->inode->mode)){
    ret = -ENOTDIR;
    goto no_dir;
  }
  //dir should be empty
  while (!ext2_readdir(file, & dirent)){
    if (strcmp(dirent.name, ".") &&
        strcmp(dirent.name, "..")){
      ret = -EEXIST;
      goto dir_not_empty;
    }
  }
  //ok, it is a empty dir, we can rm it
  entry = ext2_find_entry(file->inode->parent , name, 1);
  entry->inode = 0; //make this entry to be invalid.

  einode = file->inode->inode_data;
  einode->i_links_count = 0;
  file->inode->links_count = 0;

  ret = 0;
 no_dir:
 dir_not_empty:
  fs_close(file);
  return ret;
}

/*
 * Create a link.
 * Return 0 if successful or return error code if any error.
 *
 */
static int ext2_create_link(struct fs_inode * dir, const char *name, int inode_num, char file_type)
{
  struct ext2_dir_entry_2* entry = ext2_alloc_dir_entry(name, dir);
  if (!entry)
    return -ENOSPC;
  entry->inode = inode_num;
  entry->file_type = file_type;
  return 0;
}

/*
 * Interface of create a link.
 * Return 0 if successful or return error code if any error.
 *
 * Note: dir is not allowed to create a link.
 */
int ext2_link(const char * oldpath, const char * newpath)
{
  int ret = -1;
  char * p_path, * name;
  struct ext2_inode * einode;
  struct fs_file * old_file;
  struct fs_file * parent;

  old_file = fs_open(oldpath, O_RDONLY, 0, &ret);
  if (!old_file)
    return -ENFILE;
  einode = old_file->inode->inode_data;

  p_path = (char*)mm_kmalloc(MAX_PATH_LEN + 1);
  name = (char *)mm_kmalloc(MAX_FILE_NAME_LEN + 1);
  if (!p_path || !name){
    ret = -ENOMEM;
    goto no_mem;
  }

  if (S_ISDIR(old_file->inode->mode)){
    ret = -EISDIR;
    goto open_parent_error;
  }

  ext2_path_dir(newpath, p_path);
  ext2_path_filename(newpath, name);

  //open parent;
  if (!*name)
    return -EINVAL;
  if (!*p_path)
    parent = fs_open("./", O_RDWR, 0, &ret);
  else
    parent = fs_open(p_path, O_RDWR, 0, &ret);

  if (!parent)
    goto open_parent_error;
  if (!S_ISDIR(parent->inode->mode)){
    ret = -ENOTDIR;
    goto open_parent_error;
  }

  ret = ext2_create_link(parent->inode, name, old_file->inode->inode_num, einode->i_mode >> 12);
  if (!ret){
    einode->i_links_count++;
    old_file->inode->links_count++;
  }

  fs_close(parent);
 no_mem:
 open_parent_error:
  mm_kfree(p_path);
  mm_kfree(name);
  fs_close(old_file);
  return ret;
}

/*
 * Truncate a file to target "length".
 * Return 0 if successful or return error code if any error.
 *
 * Note: it is ok to let the length bigger than file size.
 *       and this will make hole in file, and the data in hole is all zero.
 */
int ext2_truncate(struct fs_inode* inode,off_t length)
{
  struct ext2_inode * einode = (struct ext2_inode *)inode->inode_data;
  uint32 * l1_buf = NULL, *l2_buf = NULL, *l3_buf = NULL;
  int cur_block = 0, start_block = 0;
  int ret = -1;
  int i = 0,j = 0,k = 0;
  if (length >= inode->size){
    inode->size = einode->i_size = length;
    return 0;
  }

  l1_buf = mm_kmalloc(block_size);
  if (!l1_buf)
    goto l1_buf_error;
  l2_buf = mm_kmalloc(block_size);
  if (!l2_buf)
    goto l2_buf_error;
  l3_buf = mm_kmalloc(block_size);
  if (!l3_buf)
    goto l3_buf_error;

  cur_block = start_block = (length + block_size - 1) / block_size;
  //free direct block
  for (;cur_block < LEVE0_TOTAL; cur_block++)
    if (einode->i_block[i])
      ext2_free_block(einode->i_block[i]);

  //free leve1 indirect
  if (einode->i_block[12]){
    read_block(einode->i_block[12], l1_buf);
    if (start_block <= LEVE0_TOTAL)
      ext2_free_block(einode->i_block[12]);

    for (;cur_block < LEVE1_TOTAL; cur_block++)
      if (l1_buf[cur_block - LEVE0_TOTAL])
        ext2_free_block(l1_buf[cur_block - LEVE0_TOTAL]);

  }
  //free leve2 indirect
  if (einode->i_block[13]){
    read_block(einode->i_block[13], l1_buf);
    if (start_block <= LEVE1_TOTAL)
      ext2_free_block(einode->i_block[13]);

    while (cur_block < LEVE2_TOTAL){
      j = (cur_block - LEVE1_TOTAL) / BLOCKS_PER_BLOCK;
      i = (cur_block - LEVE1_TOTAL) % BLOCKS_PER_BLOCK;
      if (!l1_buf[j]){
        cur_block += BLOCKS_PER_BLOCK;
        continue;
      }
      read_block(l1_buf[j], l2_buf);
      if (start_block <= LEVE1_TOTAL + j * BLOCKS_PER_BLOCK)
        ext2_free_block(l1_buf[j]);
      for (; i < BLOCKS_PER_BLOCK; i++, cur_block++)
        if (l2_buf[i])
          ext2_free_block(l2_buf[i]);
    }
  }
  //free leve3 indirect
  if (einode->i_block[14]){
    read_block(einode->i_block[14], l1_buf);
    if (start_block <= LEVE2_TOTAL)
      ext2_free_block(einode->i_block[14]);

    while (cur_block < LEVE3_TOTAL){
      i = (cur_block - LEVE2_TOTAL) / LEVE2_BLOCKS;
      j = ((cur_block - LEVE2_TOTAL) % LEVE2_BLOCKS) / BLOCKS_PER_BLOCK;
      if (!l1_buf[j]){
        cur_block += LEVE2_BLOCKS;
        continue;
      }
      read_block(l1_buf[j], l2_buf);
      if (start_block <= LEVE2_TOTAL + j * LEVE2_BLOCKS)
        ext2_free_block(l1_buf[j]);

      for (; i < BLOCKS_PER_BLOCK; i++){
        k = ((cur_block - LEVE2_TOTAL) % LEVE2_BLOCKS) % BLOCKS_PER_BLOCK;
        if (!l2_buf[i]){
          cur_block += BLOCKS_PER_BLOCK;
          continue;
        }
        read_block(l2_buf[i], l3_buf);
        if (start_block <= LEVE2_TOTAL + j * LEVE2_BLOCKS + i * BLOCKS_PER_BLOCK)
          ext2_free_block(l2_buf[i]);

        for (; k < BLOCKS_PER_BLOCK; k++, cur_block++)
          if (l3_buf[k])
            ext2_free_block(l3_buf[k]);
      }
    }
  }

  einode->i_size = length;
  inode->size = length;
  ret = 0;
  mm_kfree(l3_buf);
 l3_buf_error:
  mm_kfree(l2_buf);
 l2_buf_error:
  mm_kfree(l1_buf);
 l1_buf_error:
  return ret;
}

/*
 * Write file system global data to disk.
 * Such as inode bitmap, block bitmap.
 */
void ext2_sync_system()
{
  int i;
  for (i = 0; i < group_num; i++){
    write_block(fs_group_infos[i].bg_block_bitmap, block_maps[i]->map);
    write_block(fs_group_infos[i].bg_inode_bitmap, inode_maps[i]->map);
  }
}
