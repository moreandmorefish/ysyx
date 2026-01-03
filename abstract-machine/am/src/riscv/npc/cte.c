#include <am.h>
#include <riscv/riscv.h>
#include <klib.h>

static Context* (*user_handler)(Event, Context*) = NULL;

Context* __am_irq_handle(Context *c) {
  if (user_handler) {
    Event ev = {0};
    switch (c->mcause) {
      // [关键修复] 添加对 ecall (0x0b) 的处理
      case 0x0b: 
      case 0x08: // 同时也处理 User ecall (如果有的话)
        // ecall 返回地址必须 +4，跳过 ecall 指令本身，否则会死循环
        c->mepc += 4;

        // 检查 a5 (RISCV32E) 或 a7 (RISCV32) 寄存器的值来区分 Yield 和 Syscall
        // 之前我们在 riscv.h 里定义了 GPR1 宏，这里直接用
        if (c->GPR1 == -1) {
          ev.event = EVENT_YIELD; 
        } else {
          ev.event = EVENT_SYSCALL;
        }
        break;

      default: ev.event = EVENT_ERROR; break;
    }

    c = user_handler(ev, c);
    assert(c != NULL);
  }

  return c;
}

extern void __am_asm_trap(void);

bool cte_init(Context*(*handler)(Event, Context*)) {
  // initialize exception entry
  asm volatile("csrw mtvec, %0" : : "r"(__am_asm_trap));

  // register event handler
  user_handler = handler;

  return true;
}

Context *kcontext(Area kstack, void (*entry)(void *), void *arg) {
  return NULL;
}

void yield() {
#ifdef __riscv_e
  asm volatile("li a5, -1; ecall");
#else
  asm volatile("li a7, -1; ecall");
#endif
}

bool ienabled() {
  return false;
}

void iset(bool enable) {
}