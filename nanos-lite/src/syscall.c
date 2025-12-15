#include <common.h>
#include "syscall.h"

char *syscall_names[] = {
  "SYS_exit",
  "SYS_yield",
  "SYS_open",
  "SYS_read",
  "SYS_write",
  "SYS_kill",
  "SYS_getpid",
  "SYS_close",
  "SYS_lseek",
  "SYS_brk",
  "SYS_fstat",
  "SYS_time",
  "SYS_signal",
  "SYS_execve",
  "SYS_fork",
  "SYS_link",
  "SYS_unlink",
  "SYS_wait",
  "SYS_times",
  "SYS_gettimeofday"
};

size_t sys_write(int fd, const void *buf, size_t count) {
  // fd=1: stdout, fd=2: stderr
  // 目前我们只支持输出到串口，所以只处理这两个 fd
  if (fd == 1 || fd == 2) {
    const char *p = (const char *)buf;
    for (int i = 0; i < count; i++) {
      putch(p[i]); // 使用 AM 提供的 putch 输出一个字符
    }
    return count; // 返回实际写入的字节数
  }
  
  // 目前不支持其他文件描述符，暂时忽略
  return -1; 
}

void do_syscall(Context *c) {
  uintptr_t a[4];
  a[0] = c->GPR1; // syscall ID
  a[1] = c->GPR2; // arg 1
  a[2] = c->GPR3; // arg 2
  a[3] = c->GPR4; // arg 3

  // --- strace 实现开始 ---
  
  // 1. 获取系统调用名字
  char *name = "Unknown";
  // 检查是否越界，防止访问数组之外的内存
  if (a[0] >= 0 && a[0] < sizeof(syscall_names) / sizeof(syscall_names[0])) {
      name = syscall_names[a[0]];
  }

  // 2. 打印 trace 信息 (推荐使用 Log 宏，或者 printf)
  // 格式参考: [strace] SYS_name (ID) args...
  Log("strace: %s (ID=%d) args(0x%x, 0x%x, 0x%x)", 
      name, a[0], a[1], a[2], a[3]);

  // --- strace 实现结束 ---

  switch (a[0]) {
    // ... 你的 switch case 代码 ...
    case SYS_exit: //0
      halt(a[1]);
      break;
    case SYS_yield: //1
      yield();
      c->GPRx = 0;
      break;
    case SYS_write:
      // 调用辅助函数，并将返回值写入 GPRx (a0)
      // 注意参数转换：a[1]是fd, a[2]是buf指针, a[3]是长度
      c->GPRx = sys_write((int)a[1], (void *)a[2], (size_t)a[3]); 
      break;
    case SYS_brk:
      // a[1] 是新的 program break 位置
      // 目前我们总是返回 0，表示成功
      // (真正的 OS 需要记录这个值并检查是否越界，但现在先这样)
      c->GPRx = 0; 
      break;
    default: 
      panic("Unhandled syscall ID = %d", a[0]);
  }
}