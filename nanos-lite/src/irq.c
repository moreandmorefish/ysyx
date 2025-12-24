#include <common.h>
#include <am.h>

Context* do_syscall(Context *c);
// 声明外部函数 schedule
Context* schedule(Context *prev);

// #define DO_EVENT_STRACE 1

static Context* do_event(Event e, Context* c) {
  switch (e.event) {
    case EVENT_YIELD: 
      // printf("Event: yield!\n"); // 可以保留作为调试，觉得吵可以注释掉
      
      // ============ 关键修改 ============
      // 调用调度器，传入当前的上下文 c，并返回下一个要运行的上下文
      return schedule(c); 
      // ================================
      
    case EVENT_SYSCALL: 
      #ifdef DO_EVENT_STRACE
        printf("do_syscall(%d)\n", e.event); 
      #endif
      return do_syscall(c);
      break;

    default: panic("Unhandled event ID = %d", e.event);
  }

  return c;
}

void init_irq(void) {
  Log("Initializing interrupt/exception handler...");
  cte_init(do_event);
}