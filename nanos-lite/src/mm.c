#include <memory.h>
#include <proc.h>

static void *pf = NULL;
#define PGSIZE 4096
#define PGMASK (PGSIZE - 1)

// 分配 nr_page 页空闲物理内存
void* new_page(size_t nr_page) {
  // 1. 计算需要的字节数
  size_t n = nr_page * PGSIZE;
  
  // 2. 检查堆空间是否溢出 (可选，但建议加)
  if ((uintptr_t)pf + n > (uintptr_t)heap.end) {
    panic("Out of physical memory!");
  }

  // 3. 记录当前分配的起始地址
  void *ret = pf;

  // 4. 指针后移 (Bump Pointer)
  pf += n;

  // 5. [关键] 清零内存！
  // 无论是页表还是 BSS 段，都需要 0 初始化。
  memset(ret, 0, n);

  return ret;
}

#ifdef HAS_VME
static void* pg_alloc(int n) {
  // pg_alloc 也是分配 n 字节，且必须清零
  // 我们可以复用 new_page 的逻辑，或者直接在这里写
  // 这里为了逻辑独立，我们简单粗暴地写一遍
  
  // 确保 n 是 4KB (通常 AM 传进来的就是 PGSIZE)
  assert(n % PGSIZE == 0);

  if ((uintptr_t)pf + n > (uintptr_t)heap.end) {
    panic("Out of physical memory in pg_alloc!");
  }

  void *ret = pf;
  pf += n;
  memset(ret, 0, n); // 页表必须清零！
  return ret;
}
#endif

void free_page(void *p) {
  panic("not implement yet");
}

/* The brk() system call handler. */
int mm_brk(uintptr_t brk) {
  // 1. 获取当前进程的 max_brk
  // 我们总是操作当前正在运行的进程
  uintptr_t max_brk = current->max_brk;

  // 2. 如果新请求的 brk 超过了当前的 max_brk，就需要分配新页
  if (brk > max_brk) {
    // 这里的策略是：把 max_brk 到 brk 之间的所有虚拟页都分配物理页
    
    // page_start: 从 max_brk 开始
    // page_end:   向上对齐到页边界的新 brk
    uintptr_t new_brk_align = (brk + PGSIZE - 1) & ~PGMASK;

    for (uintptr_t va = max_brk; va < new_brk_align; va += PGSIZE) {
      // A. 申请一页物理内存
      void *pa = new_page(1);
      
      // B. 建立映射
      map(&current->as, (void *)va, pa, 0); // prot 暂时给 0 或 RW
    }

    // 3. 更新 max_brk
    current->max_brk = new_brk_align;
  }

  return 0;
}

void init_mm() {
  // 初始化 pf 指针，指向堆的起始位置 (向上取整对齐)
  pf = (void *)ROUNDUP(heap.start, PGSIZE);
  Log("free physical pages starting from %p", pf);

  // 显式指定物理内存的结束位置 (根据 NEMU 的配置，通常是 128MB)
  // 0x80000000 + 128MB = 0x88000000
  heap.end = (void *)0x88000000; 

#ifdef HAS_VME
  // 将分配函数注册给 AM
  vme_init(pg_alloc, free_page);
#endif
}