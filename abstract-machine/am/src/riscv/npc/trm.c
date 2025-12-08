#include <am.h>
#include <klib-macros.h>

extern char _heap_start;
int main(const char *args);

extern char _pmem_start;
#define PMEM_SIZE (128 * 1024 * 1024)
#define PMEM_END  ((uintptr_t)&_pmem_start + PMEM_SIZE)

// [修改 1] 定义串口地址，保持与 NEMU 一致
#define SERIAL_PORT 0xa00003f8

Area heap = RANGE(&_heap_start, PMEM_END);
static const char mainargs[MAINARGS_MAX_LEN] = TOSTRING(MAINARGS_PLACEHOLDER);

// [修改 2] 实现 putch 函数
void putch(char ch) {
  // 使用 volatile 关键字防止编译器优化掉写操作
  // 将字符 ch 写入串口地址
  *(volatile char *)SERIAL_PORT = ch;
}

void halt(int code) {
  __asm__ volatile("ebreak");
  while (1);
}

void _trm_init() {
  int ret = main(mainargs);
  halt(ret);
}