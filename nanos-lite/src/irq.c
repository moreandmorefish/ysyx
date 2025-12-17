#include <common.h>
#include <am.h>
void do_syscall(Context *c);
// #define DO_EVENT_STRACE 1
static Context* do_event(Event e, Context* c) {
  switch (e.event) {
    case EVENT_YIELD: printf("Event: yield!\n"); break;
    case EVENT_SYSCALL: 
    #ifdef DO_EVENT_STRACE
      printf("do_syscall(%d)\n", e.event); 
    #endif
    do_syscall(c);break;
    default: panic("Unhandled event ID = %d", e.event);
  }

  return c;
}

void init_irq(void) {
  Log("Initializing interrupt/exception handler...");
  cte_init(do_event);
}
