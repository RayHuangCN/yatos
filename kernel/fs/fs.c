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
#include <yatos/task.h>
#include <yatos/sys_call.h>
#include <yatos/task_vmm.h>
#include <yatos/schedule.h>
#include <arch/regs.h>

static struct kcache * file_cache;
static struct kcache * inode_cache;
static struct kcache * data_buffer_cache;
static struct list_head inode_list;
static struct fs_file * root_dir;


struct fs_file * fs_get_root()
{
  return root_dir;
}

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
  //if count == 0, we don't free inode now
}


static void fs_constr_file(void *arg)
{
  struct fs_file *file = (struct fs_file*)arg;
  file->count = 1;
}

static void fs_distr_file(void *arg)
{
  struct fs_file * file = (struct fs_file*)arg;
  if (file->inode && file->inode->action && file->inode->action->close)
    file->inode->action->close(file);
  fs_put_inode(file->inode);
}

static void fs_constr_inode(void *arg)
{
  struct fs_inode * inode = (struct fs_inode *)arg;
  inode->count = 1;
  INIT_LIST_HEAD(&(inode->data_buffers));
  INIT_LIST_HEAD(&(inode->list_entry));
}

static void fs_distr_inode(void * arg)
{
  struct fs_inode * inode = (struct fs_inode *)arg;
  struct list_head * cur;
  struct list_head * safe;
  struct fs_data_buffer * data;

  if (inode->action && inode->action->sync)
    inode->action->sync(inode);

  list_for_each_safe(cur, safe, &(inode->data_buffers)){
    data = container_of(cur, struct fs_data_buffer, list_entry);
    slab_free_obj(data);
  }
  if (inode->action && inode->action->release)
    inode->action->release(inode);
  if (!list_empty(&(inode->list_entry)))
      list_del(&(inode->list_entry));
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
  data_buffer_cache = slab_create_cache(sizeof(struct fs_data_buffer), NULL, fs_distr_dbuffer, "data_buffer cache");
  if (!file_cache || !inode_cache || !data_buffer_cache)
    go_die("fs_init_caches error\n\r");
}



static struct fs_inode * fs_search_inode(int num)
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
static void fs_add_inode(struct fs_inode * inode)
{
  list_add_tail(&(inode->list_entry), &inode_list);
}


void fs_close(struct fs_file* file)
{
  fs_put_file(file);
}


struct fs_data_buffer * fs_inode_get_buffer(struct fs_inode * fs_inode, unsigned long block_offset)
{
  if (!fs_inode)
    return NULL;
  //check recent buffer;
  if (fs_inode->recent_data && fs_inode->recent_data->block_offset == block_offset)
    return fs_inode->recent_data;

  //search
  struct list_head *cur;
  struct fs_data_buffer * after;
  list_for_each(cur, &(fs_inode->data_buffers)){
    after = container_of(cur, struct fs_data_buffer, list_entry);
    if (after->block_offset == block_offset){
      fs_inode->recent_data = after;
      return after;
    }
    if (after->block_offset > block_offset)
      break;
  }

  //can not found, add new
  struct fs_data_buffer * buf = slab_alloc_obj(data_buffer_cache);
  if (!buf)
    return NULL;
  buf->block_offset = block_offset;
  char * buffer = mm_kmalloc(FS_DATA_BUFFER_SIZE);
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
  fs_inode->recent_data = buf;
  return buf;
}

static int fs_gener_read(struct fs_file* file, char* buffer,unsigned long count)
{
  struct fs_inode * inode = file->inode;
  uint32 off_set = file->cur_offset;
  struct ext2_inode * ext2_inode = (struct ext2_inode *)inode->inode_data;
  if (off_set >= ext2_inode->i_size)
    return 0;
  uint32 total_size = ext2_inode->i_size;

  if (total_size - off_set < count)
    count = total_size - off_set;
  uint32 read_count = 0;
  uint32 block_offset;
  uint32 buff_offset;
  struct fs_data_buffer * buf;
  uint32 cpy_size;
  while (count){
    block_offset = off_set / FS_DATA_BUFFER_SIZE;
    buff_offset = off_set % FS_DATA_BUFFER_SIZE;
    buf = fs_inode_get_buffer(inode, block_offset);
    cpy_size = FS_DATA_BUFFER_SIZE - buff_offset;
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

static int fs_gener_write(struct fs_file* file,char* buffer,unsigned long count)
{
  struct fs_inode * inode  = file->inode;
  uint32 off_set = file->cur_offset;
  struct ext2_inode * ext2_inode = inode->inode_data;
  uint32 write_count = 0;
  uint32 block_offset;
  uint32 buff_offset;
  struct fs_data_buffer * buf;
  uint32 cpy_size;

  file->cur_offset += count;
  if (count && file->cur_offset > ext2_inode->i_size)
    ext2_inode->i_size = file->cur_offset;

  while (count){
    block_offset = off_set / FS_DATA_BUFFER_SIZE;
    buff_offset = off_set % FS_DATA_BUFFER_SIZE;
    buf = fs_inode_get_buffer(inode, block_offset);
    BUFFER_SET_DIRTY(buf);
    cpy_size = FS_DATA_BUFFER_SIZE - buff_offset;
    if (cpy_size > count)
      cpy_size = count;

    memcpy(buf->buffer + buff_offset , buffer + write_count, cpy_size);
    off_set += cpy_size;
    write_count += cpy_size;
    count -= cpy_size;
  }
  return write_count;
}

static void fs_gener_sync(struct fs_inode  *inode)
{
  ext2_sync_data(inode);
}

static void fs_gener_release(struct fs_inode * inode)
{
  ext2_release_inode((struct ext2_inode*)inode->inode_data);
}

static int fs_gener_readdir(struct fs_file * file, struct kdirent *ret)
{
  struct fs_inode * inode = file->inode;
  if (!S_ISDIR(inode->mode))
    return -1;
  return ext2_readdir(file, ret);
}

static off_t fs_gener_seek(struct fs_file* file,off_t offset,int whence)
{

  struct ext2_inode * ext2_inode = file->inode->inode_data;
  switch(whence){
  case SEEK_SET:
    file->cur_offset = offset;
    return file->cur_offset;
  case SEEK_CUR:
    file->cur_offset += offset;
    if (file->cur_offset < 0)
      file->cur_offset = 0;
    return file->cur_offset;
  case SEEK_END:
    file->cur_offset = ext2_inode->i_size + offset;
    return file->cur_offset;
  }
  return -1;
}

static struct fs_inode_oper gerner_inode_oper = {
  .read = fs_gener_read,
  .write = fs_gener_write,
  .seek = fs_gener_seek,
  .sync = fs_gener_sync,
  .release = fs_gener_release,
  .readdir = fs_gener_readdir
};

struct fs_file * fs_open(const char * path, int flag, mode_t mode)
{
  //check cur dir
  struct task * task = task_get_cur();
  struct fs_inode * cur_inode;
  int i = 0;

  while (path[i] && (path[i] == ' ' || path[i] == '\t'))
    i++;
  if (!path || !path[i])
    return NULL;

  if (path[i] == '/')
    cur_inode = root_dir->inode;
  else
    cur_inode = task->cur_dir->inode;
  // now open
  char name[MAX_FILE_NAME_LEN];
  int p = 0;
  const char * cur_c = path;
  struct ext2_inode * ext2_inode = cur_inode->inode_data;
  int inode_num;

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

    if (!(ext2_inode->i_mode & EXT2_IFDIR)){
      printk("not a dir\n\r");
      return NULL;
    }
    if ((inode_num = ext2_find_file(name, cur_inode)) == -1){
      const char * tmp = cur_c;
      while (*tmp && *tmp == '/')
        tmp++;
      if (*tmp) //this means what we not find is just a mid dir
        return NULL;
      if ((flag & O_CREAT) && !(flag & O_EXCL)){
        if ((inode_num = ext2_create_file(name, cur_inode, mode)) == -1)
            return NULL;
      }else
        return NULL;
    }
    cur_inode  = fs_search_inode(inode_num);
    if (!cur_inode){
      cur_inode = slab_alloc_obj(inode_cache);
      if (!cur_inode)
        return NULL;
      ext2_fill_inode(cur_inode, inode_num);
      //make count to be zero since we don't change inode count during searching inode
      fs_put_inode(cur_inode);
      //add to inode cache
      fs_add_inode(cur_inode);
     }
    ext2_inode = cur_inode->inode_data;
  }

  struct fs_file * ret_file = (struct fs_file *)slab_alloc_obj(file_cache);
  if (!ret_file)
    return NULL;
  ret_file->inode = cur_inode;
  cur_inode->action = &gerner_inode_oper;
  ret_file->flag = flag;
  //now we should incre inode count
  fs_get_inode(cur_inode);
  return ret_file;
}


int fs_read(struct fs_file* file,char* buffer,unsigned long count)
{
  if (file->inode->action->read)
    return file->inode->action->read(file, buffer, count);
  else
    return -1;
}


int fs_write(struct fs_file * file, char * buffer, unsigned long count)
{
  if (file->inode->action->write)
    return file->inode->action->write(file, buffer, count);
  else
    return -1;
}

void fs_sync(struct fs_file * file)
{
  if (file->inode->action->sync)
    file->inode->action->sync(file->inode);
}

off_t fs_seek(struct fs_file * file, off_t offset, int whence)
{
  if (file->inode->action->seek)
    return file->inode->action->seek(file, offset, whence);
  else
    return -1;
  return 0;
}


static int sys_call_open(struct pt_regs * regs)
{
  const char * path = (const char *)sys_call_arg1(regs);
  int flag = (int)sys_call_arg2(regs);
  mode_t mode = (int)sys_call_arg3(regs);

  struct task * task = task_get_cur();
  struct fs_file * file;
  int fd;

  char * tmp_buffer = (char *)mm_kmalloc(PAGE_SIZE);
  if (!tmp_buffer)
    return -1;

  if (task_copy_str_from_user(tmp_buffer, path, MAX_PATH_LEN))
    goto copy_from_user_error;

  file = fs_open(tmp_buffer, flag, mode);
  if (!file)
    goto fs_open_error;

  fd = bitmap_alloc(task->fd_map);
  if (fd >= 0){
    mm_kfree(tmp_buffer);
    task->files[fd] = file;
    return fd;
  }

 fs_open_error:
 copy_from_user_error:
  mm_kfree(tmp_buffer);
  return -1;
}

static int sys_call_read(struct pt_regs * regs)
{
  int fd = (int)sys_call_arg1(regs);
  if (fd < 0 || fd >= MAX_OPEN_FD)
    return -1;
  char *buffer = (char *)sys_call_arg2(regs);
  if (!buffer)
    return -1;
  unsigned long size = (unsigned long)sys_call_arg3(regs);

  struct task * task = task_get_cur();
  struct fs_file * file = task->files[fd];
  if (!file || !file->inode || !file->inode->action->read)
    return -1;

  char * tmp_buffer = (char *)mm_kmalloc(size);
  if (!tmp_buffer)
    return -1;

  int read_n = file->inode->action->read(file, tmp_buffer, size);
  if (read_n > 0){
    if (task_copy_to_user(buffer, tmp_buffer, read_n))
      read_n = -1;
  }
  mm_kfree(tmp_buffer);
  return read_n;
}

static int sys_call_write(struct pt_regs *regs)
{
  int fd = (int)sys_call_arg1(regs);
  if (fd < 0 || fd >= MAX_OPEN_FD)
    return -1;
  char *buffer = (char *)sys_call_arg2(regs);
  if (!buffer)
    return -1;
  unsigned long size = (unsigned long)sys_call_arg3(regs);

  struct task * task = task_get_cur();
  struct fs_file * file = task->files[fd];
  if (!file || !file->inode || !file->inode->action->write)
    return -1;

  char * tmp_buffer = (char *)mm_kmalloc(size);
  if (!tmp_buffer)
    return -1;

  if (task_copy_from_user(tmp_buffer, buffer, size)){
    mm_kfree(tmp_buffer);
    return -1;
  }

  int write_n = file->inode->action->write(file, tmp_buffer, size);
  mm_kfree(tmp_buffer);
  return write_n;
}


static int sys_call_seek(struct pt_regs * regs)
{
  int fd = (int)sys_call_arg1(regs);
  if (fd < 0 || fd >= MAX_OPEN_FD)
    return -1;
  off_t offset = (off_t)sys_call_arg2(regs);
  int whence = (int)sys_call_arg3(regs);

  struct task * task = task_get_cur();
  struct fs_file * file = task->files[fd];
  if (!file || !file->inode || !file->inode->action->seek)
    return -1;

  return file->inode->action->seek(file, offset, whence);
}

static int sys_call_sync(struct pt_regs *regs)
{
  int fd = (int)sys_call_arg1(regs);
  if (fd < 0 || fd >= MAX_OPEN_FD)
    return -1;
  struct task *task = task_get_cur();
  struct fs_file * file = task->files[fd];
  if (!file || !file->inode || !file->inode->action->sync)
    return -1;
  file->inode->action->sync(file->inode);
  return 0;
}

static int sys_call_close(struct pt_regs * regs)
{
  int fd = (int)sys_call_arg1(regs);
  if (fd < 0 || fd >= MAX_OPEN_FD)
    return -1;
  struct task * task = task_get_cur();
  struct fs_file * file = task->files[fd];
  fs_put_file(file);
  task->files[fd] = NULL;
  bitmap_free(task->fd_map, fd);
  return 0;
}

static int sys_call_ioctl(struct pt_regs * regs)
{
  int fd  = (int)sys_call_arg1(regs);
  int requst = (int)sys_call_arg2(regs);
  unsigned long arg = (unsigned long)sys_call_arg3(regs);

  if (fd < 0 || fd >= MAX_OPEN_FD)
    return -1;

  struct task * task = task_get_cur();
  struct fs_file * file = task->files[fd];

  if (!file || !file->inode || !file->inode->action->ioctl)
    return -1;

  return file->inode->action->ioctl(file, requst, arg);
}

static int sys_call_readdir(struct pt_regs * regs)
{
  int fd = (int)sys_call_arg1(regs);
  struct kdirent * ret_buffer = (struct kdirent *)sys_call_arg2(regs);
  if (fd < 0 || fd >= MAX_OPEN_FD)
    return -1;
  struct task * task = task_get_cur();
  struct fs_file * file = task->files[fd];

  if (!file || !file->inode || !file->inode->action->readdir)
    return -1;
  struct kdirent tmp_buffer;
  int ret;
  if (!(ret = file->inode->action->readdir(file, &tmp_buffer))){
    if (task_copy_to_user((char *)ret_buffer, (char*)&tmp_buffer, sizeof(struct kdirent)))
      return -1;
  }
  return ret;
}

void fs_init()
{

  fs_init_caches();
  INIT_LIST_HEAD(&(inode_list));
  ext2_init();
  root_dir = slab_alloc_obj(file_cache);
  root_dir->inode = slab_alloc_obj(inode_cache);
  root_dir->inode->action = &gerner_inode_oper;
  ext2_init_root(root_dir->inode);
  fs_add_inode(root_dir->inode);

  sys_call_regist(SYS_CALL_OPEN, sys_call_open);
  sys_call_regist(SYS_CALL_READ, sys_call_read);
  sys_call_regist(SYS_CALL_WRITE, sys_call_write);
  sys_call_regist(SYS_CALL_SEEK, sys_call_seek);
  sys_call_regist(SYS_CALL_SYNC, sys_call_sync);
  sys_call_regist(SYS_CALL_CLOSE, sys_call_close);
  sys_call_regist(SYS_CALL_IOCTL, sys_call_ioctl);
  sys_call_regist(SYS_CALL_READDIR, sys_call_readdir);
}

struct fs_file * fs_new_file()
{
  return slab_alloc_obj(file_cache);
}
struct fs_inode * fs_new_inode()
{
  return slab_alloc_obj(inode_cache);
}
