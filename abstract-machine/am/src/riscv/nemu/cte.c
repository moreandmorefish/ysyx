#include <am.h>
#include <riscv/riscv.h>
#include <klib.h>

static Context* (*user_handler)(Event, Context*) = NULL;

Context* __am_irq_handle(Context *c) {
  //printf("c->mcause is %d\n", c->mcause);
  //printf("a7 is %x\n", c->gpr[17]);
  if (user_handler) {
    Event ev = {0};
    switch (c->mcause) {
      case 11: 
        if (c->GPR1 == -1) 
        {  // a7=-1 → yield自陷
            ev.event = EVENT_YIELD;
        } else {  // a7=其他值 → 系统调用
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

// am/src/riscv/nemu/cte.c

Context *kcontext(Area kstack, void (*entry)(void *), void *arg) {
  // 1. 找到栈顶
  // kstack.end 是栈的结束地址。栈是向下生长的。
  // 我们需要在栈顶预留一个 Context 结构体的大小。
  Context *c = (Context*)kstack.end - 1;

  // 2. 设置 PC (mepc)
  // 当这个 Context 被恢复时，CPU 会跳转到 mepc 指向的地址。
  // 这里我们要让它跳到内核线程的入口函数 entry。
  c->mepc = (uintptr_t)entry;

  // 3. 设置状态寄存器 (mstatus)
  // 这里的关键是 MPIE (Machine Previous Interrupt Enable) 位。
  // RISC-V 中 mstatus 的 MPIE 通常是第 7 位 (0x80)。
  // 设置为 1 意味着：当执行 mret 从异常返回时，CPU 会把 MPIE 恢复给 MIE，
  // 从而开启中断。如果这里是 0，线程运行后中断就被关死了，甚至无法 yield。
  c->mstatus = 0x1800 | 0x80; 
  // 解释：0x1800 设置 MPP=11 (Machine Mode), 0x80 设置 MPIE=1。
  // 简单写 c->mstatus = 0x80; 在 NEMU 现在的简单环境通常也行，但严谨点更好。

  // 4. 设置参数 (a0 / gpr[10])
  // 根据 RISC-V 调用规范，函数第一个参数放在 a0 寄存器。
  // 在你的 riscv.h 中，GPR2 被定义为 gpr[10]，对应 a0。
  c->gpr[10] = (uintptr_t)arg;

  // 5. (可选) 设置返回地址 ra / gpr[1]
  // 这是一个好习惯：如果 entry 函数不小心 return 了，它会跳到哪里？
  // 我们可以把它指向一个死循环或者是系统停机函数，防止跑飞。
  // c->gpr[1] = (uintptr_t)some_halt_function; 

  // 6. 返回在这个栈上构造好的 Context 指针
  return c;
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
