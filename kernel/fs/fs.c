/*************************************************
 *   Author: Ray Huang
 *   Date  : 2017/4/9
 *   Email : rayhuang@126.com
 *   Desc  : VFS
 ************************************************/
#include <yatos/list.h>
#include <yatos/ext2.h>
#include <yatos/fs.h>
#include <yatos/mm.h>
#include <yatos/ext2.h>

static struct kcache * file_cache;
static struct kcache * inode_cache;
static struct kcache * data_buffer_cache;
static struct list_head inode_list;
struct fs_inode * root_dir;

void fs_get_file(struct fs_file * file)
{
  if (!file)
    return ;
  file->count++;
}
void fs_put_file(struct fs_file* file)
{
  if (!file)
    return;

  file->count--;
  if (!file->count)
    slab_free_obj(file);
}

void fs_get_inode(struct fs_inode * inode)
{
  if (!inode)
    return ;
  inode->count++;
}

void fs_put_inode(struct fs_inode * inode)
{
  if (!inode)
    return ;
  inode->count--;
  if (!inode->count){
    slab_free_obj(inode);
  }
}


static void fs_constr_file(void *arg)
{
  struct fs_file *file = (struct fs_file*)arg;
  file->count = 1;
  file->cur_offset = 0;
  file->inode = NULL;
  file->open_flag = 0;
}

static void fs_distr_file(void *arg)
{
  struct fs_file * file = (struct fs_file*)arg;
  fs_put_inode(file->inode);
}

static void fs_constr_inode(void *arg)
{
  struct fs_inode * inode = (struct fs_inode *)arg;
  inode->count = 1;
  INIT_LIST_HEAD(&(inode->data_buffers));
  inode->inode_data = NULL;
  inode->inode_num = 0;
  INIT_LIST_HEAD(&(inode->list_entry));
}

static void fs_distr_inode(void * arg)
{
  struct fs_inode * inode = (struct fs_inode *)arg;
  struct list_head * cur;
  struct list_head * safe;
  struct fs_data_buffer * data;

  ext2_sync_data(inode);

  list_for_each_safe(cur, safe, &(inode->data_buffers)){
    data = container_of(cur, struct fs_data_buffer, list_entry);
    slab_free_obj(data);
  }
  if (inode->inode_data)
    ext2_free_inode(inode->inode_data);
  if (!list_empty(&(inode->list_entry)))
      list_del(&(inode->list_entry));
}


static void fs_constr_dbuffer(void *arg)
{
  struct fs_data_buffer * buffer = (struct fs_data_buffer *)arg;
  buffer->flag = 0;
  buffer->buffer = NULL;
  buffer->block_offset = 0;
  buffer->ext2_block_num = 0;
}
static void fs_distr_dbuffer(void *arg)
{
  struct fs_data_buffer * buffer = (struct fs_data_buffer *)arg;
  if (buffer->buffer)
    mm_kfree(buffer->buffer);
}


static void fs_init_caches()
{
  file_cache = slab_create_cache(sizeof(struct fs_file), fs_constr_file , fs_distr_file, "fs_file cache");
  inode_cache = slab_create_cache(sizeof(struct fs_inode), fs_constr_inode, fs_distr_inode, "fs_inode cache");
  data_buffer_cache = slab_create_cache(sizeof(struct fs_data_buffer), fs_constr_dbuffer, fs_distr_dbuffer, "data_buffer cache");
  if (!file_cache || !inode_cache || !data_buffer_cache)
    go_die("fs_init_caches error\n\r");
}

void fs_init()
{

  fs_init_caches();
  INIT_LIST_HEAD(&(inode_list));
  ext2_init();
  root_dir = slab_alloc_obj(inode_cache);
  ext2_init_root(root_dir);
}

static struct fs_inode * fs_search_inode_from_list(int num)
{
  struct list_head *cur;
  struct fs_inode * ret;
  list_for_each(cur, &inode_list){
    ret = container_of(cur, struct fs_inode, list_entry);
    if (ret->inode_num == num)
      return ret;
  }
  return NULL;
}

struct fs_file * fs_open(const char *path, struct fs_inode * cur_inode)
{
  char name[MAX_FILE_NAME_LEN];
  int p = 0;
  const char * cur_c = path;
  struct fs_inode * temp_fs_inode = cur_inode;
  while (*cur_c){
    p = 0;
    while (*cur_c && *cur_c == '/')
      cur_c++;
    while (*cur_c && *cur_c != '/'){
      name[p++] = *cur_c;
      cur_c++;
    }
    name[p] = '\0';

    if (!p || !strcmp(name, "."))
      continue;

    if (!(temp_fs_inode->inode_data->i_mode & EXT2_IFDIR)){
      printk("not a dir\n\r");
      return NULL;
    }
    struct fs_inode * ret = slab_alloc_obj(inode_cache);
    if (!ret)
      return NULL;
    if (!ext2_find_file(name, temp_fs_inode, ret)){
      printk("can not find %s\n\r", name);
      return NULL;
    }

    if (temp_fs_inode != cur_inode)
      fs_put_inode(temp_fs_inode);
    temp_fs_inode  = ret;
  }


  struct fs_file * ret_file = (struct fs_file *)slab_alloc_obj(file_cache);
  ret_file->inode = fs_search_inode_from_list(temp_fs_inode->inode_num);

  if (ret_file->inode){
    fs_get_inode(ret_file->inode);
    fs_put_inode(temp_fs_inode);
  }
  else{
    ret_file->inode = temp_fs_inode;
    list_add_tail(&(temp_fs_inode->list_entry), &inode_list);
  }
  return ret_file;
}


void fs_close(struct fs_file* file)
{
  fs_put_file(file);
}


static struct fs_data_buffer * fs_inode_get_buffer(struct fs_inode * fs_inode, unsigned long block_offset)
{
  if (!fs_inode)
    return NULL;

  //search
  struct list_head *cur;
  struct fs_data_buffer * after;
  list_for_each(cur, &(fs_inode->data_buffers)){
    after = container_of(cur, struct fs_data_buffer, list_entry);
    if (after->block_offset == block_offset)
      return after;
    if (after->block_offset > block_offset)
      break;
  }

  //can not foud, add new
  struct fs_data_buffer * buf = slab_alloc_obj(data_buffer_cache);
  if (!buf)
    return NULL;
  buf->block_offset = block_offset;
  char * buffer = mm_kmalloc(ext2_get_block_size());
  if (!buffer){
    slab_free_obj(buf);
    return NULL;
  }
  buf->buffer = buffer;
  if (ext2_fill_buffer(fs_inode, buf)){
      mm_kfree(buffer);
      slab_free_obj(buf);
      return NULL;
  }
  list_add(&(buf->list_entry), cur->prev);
  return buf;
}

int fs_read(struct fs_file* file,char* buffer,unsigned long count)
{
  struct fs_inode * inode = file->inode;
  uint32 off_set = file->cur_offset;
  if (off_set >= inode->inode_data->i_size)
    return 0;
  uint32 total_size = inode->inode_data->i_size;

  if (total_size - off_set < count)
    count = total_size - off_set;
  uint32 read_count = 0;
  uint32 block_offset;
  uint32 buff_offset;
  struct fs_data_buffer * buf;
  uint32 cpy_size;
  uint32 buffer_size = ext2_get_block_size();
  while (count){
    block_offset = off_set / buffer_size * buffer_size;
    buff_offset = off_set % buffer_size;
    buf = fs_inode_get_buffer(inode, block_offset);
    cpy_size = buffer_size - buff_offset;
    if (cpy_size > count)
      cpy_size = count;

    memcpy(buffer + read_count , buf->buffer + buff_offset, cpy_size);
    off_set += cpy_size;
    count -= cpy_size;
    read_count += cpy_size;
  }
  file->cur_offset += read_count;
  return read_count;
}

int fs_write(struct fs_file* file,char* buffer,unsigned long count)
{
  struct fs_inode * inode  = file->inode;
  uint32 off_set = file->cur_offset;
  if (off_set > inode->inode_data->i_size)
    inode->inode_data->i_size = off_set;

  uint32 write_count = 0;
  uint32 block_offset;
  uint32 buff_offset;
  struct fs_data_buffer * buf;
  uint32 cpy_size;
  uint32 buffer_size = ext2_get_block_size();
  while (count){
    block_offset = off_set / buffer_size * buffer_size;
    buff_offset = off_set % buffer_size;
    buf = fs_inode_get_buffer(inode, block_offset);
    cpy_size = buffer_size - buff_offset;
    if (cpy_size > count)
      cpy_size = count;

    memcpy(buf->buffer + buff_offset , buffer + write_count, cpy_size);
    off_set += cpy_size;
    write_count += cpy_size;
    count -= cpy_size;
  }
  file->cur_offset += write_count;
  return write_count;
}

void fs_sync(struct fs_file *file)
{
  ext2_sync_data(file->inode);
}
