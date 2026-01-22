#include <am.h>
#include <riscv/riscv.h>
#include <klib.h>

static Context* (*user_handler)(Event, Context*) = NULL;

Context* __am_irq_handle(Context *c) {
  if (user_handler) {
    Event ev = {0};
    switch (c->mcause) {
      case 11: 
      case  8:
        if (c->GPR1 == -1) {  // a7=-1 → yield自陷
            ev.event = EVENT_YIELD;
        } else {  // a7=其他值 → 系统调用
            ev.event = EVENT_SYSCALL;
        }
        c->mepc += 4;
        break;
      case 0x80000007: 
        ev.event = EVENT_IRQ_TIMER;
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

#include <string.h>
extern char __global_pointer$;
Context *kcontext(Area kstack, void (*entry)(void *), void *arg) {
  Context *c = (Context*)kstack.end - 1;
  
  // 1. 必须保留 memset (清除垃圾数据)
  memset(c, 0, sizeof(Context));

  c->mepc = (uintptr_t)entry;
  c->mstatus = 0x1800 | 0x80;
  c->gpr[10] = (uintptr_t)arg;
  c->gpr[2] = (uintptr_t)c;
  asm volatile("mv %0, gp" : "=r"(c->gpr[3]));
  // 2. 必须保留 pdir = NULL
  c->pdir = NULL;

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
