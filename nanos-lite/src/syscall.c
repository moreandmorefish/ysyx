#include <common.h>
#include "syscall.h"

void do_syscall(Context *c) {
  uintptr_t a[4];
  // 这里的 GPRx 宏定义在 abstract-machine/am/include/arch/riscv32-nemu.h 中
  // 如果你的 vscode 提示找不到，可以直接看那个文件
  a[0] = c->GPR1; // a7: syscall ID
  a[1] = c->GPR2; // a0: 1st argument
  a[2] = c->GPR3; // a1: 2nd argument
  a[3] = c->GPR4; // a2: 3rd argument
printf("Syscall ID = %d, Args: %d, %d, %d\n", a[0], a[1], a[2], a[3]);
  switch (a[0]) {
    case SYS_exit://0
      // 这里处理 exit。
      // Log("System call: exit, status = %d", a[1]);
      // 目前直接 halt 停机即可，status 参数暂时可以忽略，或者传给 halt
      halt(a[1]); 
      break;

    case SYS_yield://1
      // 这里处理 yield。
      // Log("System call: yield");
      yield(); 
      c->GPRx = 0; // 设置系统调用返回值为 0 (RISC-V 中 GPRx 通常是 a0)
      break;

    default: panic("Unhandled syscall ID = %d", a[0]);
  }
}