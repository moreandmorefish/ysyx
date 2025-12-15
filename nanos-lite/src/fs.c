#include <fs.h>

// 声明 ramdisk 的读写函数 (定义在 src/ramdisk.c)
size_t ramdisk_read(void *buf, size_t offset, size_t len);
size_t ramdisk_write(const void *buf, size_t offset, size_t len);
size_t serial_write(const void *buf, size_t offset, size_t len);
size_t events_read(void *buf, size_t offset, size_t len);

typedef size_t (*ReadFn) (void *buf, size_t offset, size_t len);
typedef size_t (*WriteFn) (const void *buf, size_t offset, size_t len);

typedef struct {
  char *name;
  size_t size;
  size_t disk_offset;
  ReadFn read;
  WriteFn write;
  size_t open_offset; // [新增] 添加文件读写偏移量
} Finfo;

enum {FD_STDIN, FD_STDOUT, FD_STDERR, FD_EVENTS, FD_FB};

size_t invalid_read(void *buf, size_t offset, size_t len) {
  panic("should not reach here");
  return 0;
}

size_t invalid_write(const void *buf, size_t offset, size_t len) {
  panic("should not reach here");
  return 0;
}



/* This is the information about all files in disk. */
static Finfo file_table[] __attribute__((used)) = {
  [FD_STDIN]  = {"stdin", 0, 0, invalid_read, invalid_write},
  // 修改这里：注册 serial_write
  [FD_STDOUT] = {"stdout", 0, 0, invalid_read, serial_write},
  [FD_STDERR] = {"stderr", 0, 0, invalid_read, serial_write},
  [FD_EVENTS] = {"/dev/events", 0, 0, events_read, invalid_write},
#include "files.h"
};

// 计算 file_table 中文件的数量
#define NR_FILES (sizeof(file_table) / sizeof(file_table[0]))

void init_fs() {
  // TODO: initialize the size of /dev/fb
}

// 1. fs_open: 查找文件并重置偏移量
int fs_open(const char *pathname, int flags, int mode) {
  for (int i = 0; i < NR_FILES; i++) {
    if (strcmp(file_table[i].name, pathname) == 0) {
      // 每次打开文件，必须将偏移量重置为 0
      file_table[i].open_offset = 0; 
      return i; // 返回文件描述符 (下标)
    }
  }
  panic("File not found: %s", pathname);
  return -1;
}

// 2. fs_read: 读取文件
size_t fs_read(int fd, void *buf, size_t len) {
  // 越界检查
  if (fd < 0 || fd >= NR_FILES) return 0;
  
  Finfo *f = &file_table[fd];

  // 如果有自定义的 read 函数 (用于特殊设备，如 /dev/events)，优先调用
  if (f->read) {
    return f->read(buf, f->open_offset, len);
  }

  // SFS 边界检查: 不要读出文件末尾
  if (f->open_offset + len > f->size) {
    len = f->size - f->open_offset;
  }

  // 调用 ramdisk_read 进行真正的数据读取
  // 物理偏移 = 文件在磁盘的起始位置 + 当前偏移量
  ramdisk_read(buf, f->disk_offset + f->open_offset, len);
  
  // 更新偏移量
  f->open_offset += len;
  return len;
}

// 3. fs_write: 写文件
size_t fs_write(int fd, const void *buf, size_t len) {
  if (fd < 0 || fd >= NR_FILES) return 0;

  Finfo *f = &file_table[fd];

  // [修改] 优先使用设备特定的 write 函数
  // 这样 stdout/stderr 就会自动调用 serial_write
  if (f->write) {
    return f->write(buf, f->open_offset, len);
  }
  
  // [删除] 删掉原来这里关于 fd==1 || fd==2 的 if 语句块

  // 普通文件的写入逻辑保持不变
  size_t write_len = len;
  if (f->open_offset + len > f->size) {
    write_len = f->size - f->open_offset;
  }
  ramdisk_write(buf, f->disk_offset + f->open_offset, write_len);
  f->open_offset += write_len;
  return write_len;
}

// 4. fs_lseek: 调整偏移量
size_t fs_lseek(int fd, size_t offset, int whence) {
  if (fd < 0 || fd >= NR_FILES) return -1;
  
  Finfo *f = &file_table[fd];
  size_t new_offset = f->open_offset;

  switch (whence) {
    case SEEK_SET: // 也就是 0
      new_offset = offset;
      break;
    case SEEK_CUR: // 也就是 1
      new_offset = f->open_offset + offset;
      break;
    case SEEK_END: // 也就是 2
      new_offset = f->size + offset;
      break;
    default:
      panic("Invalid whence: %d", whence);
  }

  // 边界检查: offset 允许等于 size (指向文件末尾)，但不允许越界
  if (new_offset < 0 || new_offset > f->size) {
    // 真实的 lseek 可能会返回错误，这里我们简单处理，或者 panic
    // panic("lseek out of bounds");
    // 暂时允许稍微越界，或者根据需要截断。这里保持原样返回
  }
  
  f->open_offset = new_offset;
  return new_offset;
}

// 5. fs_close: 关闭文件
int fs_close(int fd) {
  // 由于 SFS 没有维护打开状态，直接返回 0 即可
  return 0;
}