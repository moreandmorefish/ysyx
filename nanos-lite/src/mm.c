#include <memory.h>
#include <proc.h> 

static void *pf = NULL;

void* new_page(size_t nr_page) {
  // 目前不需要实现
  return NULL;
}

#ifdef HAS_VME
static void* pg_alloc(int n) {
  return NULL;
}
#endif

void free_page(void *p) {
  panic("not implement yet");
}

/* The brk() system call handler. */
int mm_brk(uintptr_t brk) {
  // 可以在这里加个检查，防止堆溢出
  // if (brk > (uintptr_t)heap.end) return -1;
  return 0;
}

void init_mm() {
  pf = (void *)ROUNDUP(heap.start, PGSIZE);
  Log("free physical pages starting from %p", pf);

  // [建议] 显式指定堆的终点是物理内存末尾
  // RISC-V NEMU 默认内存通常到 0x88000000
  heap.end = (void *)0x88000000; 

#ifdef HAS_VME
  vme_init(pg_alloc, free_page);
#endif
}