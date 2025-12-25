#include <common.h>
#include "syscall.h"
#include "fs.h"
#include <proc.h>

extern PCB *current;
void context_uload(PCB *pcb, const char *filename, char *const argv[]);
void switch_boot_pcb();


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

// --- [新增] 手动定义 timeval 结构体 ---
// 这是为了匹配 Newlib 中定义的 struct timeval 内存布局
struct timeval {
  long tv_sec;     /* seconds */
  long tv_usec;    /* microseconds */
};

struct timezone {
  int tz_minuteswest;     /* minutes west of Greenwich */
  int tz_dsttime;         /* type of DST correction */
};

int sys_gettimeofday(struct timeval *tv, struct timezone *tz) {
  // 1. 定义变量
  AM_TIMER_UPTIME_T uptime;
  
  // 2. 传入地址
  ioe_read(AM_TIMER_UPTIME, &uptime);
  
  // 3. 从结构体中取值
  uint64_t us = uptime.us;
  
  if (tv != NULL) {
    tv->tv_sec = us / 1000000;
    tv->tv_usec = us % 1000000;
  }
  return 0;
}
// ------------------------------------

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

Context* schedule(Context *prev);

//#define STRACE 1
Context* do_syscall(Context *c) {
  uintptr_t a[4];
  a[0] = c->GPR1; // syscall ID
  a[1] = c->GPR2; // arg 1
  a[2] = c->GPR3; // arg 2
  a[3] = c->GPR4; // arg 3
  #if STRACE
    char *name = "Unknown";
    if (a[0] >= 0 && a[0] < sizeof(syscall_names) / sizeof(syscall_names[0])) {
        name = syscall_names[a[0]];
    }

    Log("strace: %s (ID=%d) args(0x%x, 0x%x, 0x%x)", name, a[0], a[1], a[2], a[3]);
  #endif

  switch (a[0]) {
    case SYS_exit:
      naive_uload(NULL, "/bin/menu");
      c->GPRx = 0;
      halt(a[1]);
      break;

    case SYS_yield:
      return schedule(c);
      c->GPRx = 0;
      break;

    case SYS_open:
      // fs_open(filename, flags, mode)
      c->GPRx = fs_open((const char *)a[1], a[2], a[3]);
      break;

    case SYS_read:
      // fs_read(fd, buf, len)
      c->GPRx = fs_read(a[1], (void *)a[2], a[3]);
      break;

    case SYS_write:
      // fs_write(fd, buf, len)
      // 注意：串口输出已经在 fs.c 的 fs_write 中处理了，这里只需透传
      c->GPRx = fs_write(a[1], (void *)a[2], a[3]);
      break;

    case SYS_close:
      // fs_close(fd)
      c->GPRx = fs_close(a[1]);
      break;

    case SYS_lseek:
      // fs_lseek(fd, offset, whence)
      c->GPRx = fs_lseek(a[1], a[2], a[3]);
      break;

    case SYS_brk:
      c->GPRx = 0; 
      break;

    case SYS_gettimeofday:
      // 强转指针类型
      c->GPRx = sys_gettimeofday((struct timeval *)a[1], (struct timezone *)a[2]);
      break;
    case SYS_execve:
      // a[1]: filename, a[2]: argv, a[3]: envp (目前 context_uload 暂未处理 envp，可忽略)
      if (a[1] != 0) {
        // [修复] 使用 context_uload 加载新程序
        // 这会重置 current 进程的栈，构造参数，并更新 trapframe (cp)
        context_uload(current, (const char *)a[1], (char *const *)a[2]);
        
        // [关键] 返回新构造的上下文 (current->cp)
        // 这样当 trap.S 恢复现场时，就会切换到新程序的入口和新栈
        return current->cp; 
      } else {
        c->GPRx = -1;
      }
      break;

    default: 
      panic("Unhandled syscall ID = %d", a[0]);
  }
  return c;
}