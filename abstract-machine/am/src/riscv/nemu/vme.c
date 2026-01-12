#include <am.h>
#include <nemu.h>
#include <klib.h>

static AddrSpace kas = {};
static void* (*pgalloc_usr)(int) = NULL;
static void (*pgfree_usr)(void*) = NULL;
static int vme_enable = 0;

static Area segments[] = {      // Kernel memory mappings
  NEMU_PADDR_SPACE
};

#define USER_SPACE RANGE(0x40000000, 0x80000000)

#define PG_ALIGN __attribute((aligned(PGSIZE)))

// 页表项 (PTE) 字段掩码
#define PTE_V 0x01
#define PTE_R 0x02
#define PTE_W 0x04
#define PTE_X 0x08
#define PTE_U 0x10

// 辅助宏：提取虚拟页号
#define PDX(va)     (((uintptr_t)(va) >> 22) & 0x3ff) // 一级索引
#define PTX(va)     (((uintptr_t)(va) >> 12) & 0x3ff) // 二级索引
#define PPN(pte)    (((uintptr_t)(pte) >> 10) << 12)  // 从 PTE 取出物理页基址

static inline void set_satp(void *pdir) {
  uintptr_t mode = 1ul << (__riscv_xlen - 1);
  asm volatile("csrw satp, %0" : : "r"(mode | ((uintptr_t)pdir >> 12)));
}

static inline uintptr_t get_satp() {
  uintptr_t satp;
  asm volatile("csrr %0, satp" : "=r"(satp));
  return satp << 12;
}

bool vme_init(void* (*pgalloc_f)(int), void (*pgfree_f)(void*)) {
  pgalloc_usr = pgalloc_f;
  pgfree_usr = pgfree_f;

  kas.ptr = pgalloc_f(PGSIZE);

  int i;
  for (i = 0; i < LENGTH(segments); i ++) {
    void *va = segments[i].start;
    for (; va < segments[i].end; va += PGSIZE) {
      map(&kas, va, va, 0);
    }
  }

  set_satp(kas.ptr);
  vme_enable = 1;

  return true;
}

void protect(AddrSpace *as) {
  PTE *updir = (PTE*)(pgalloc_usr(PGSIZE));
  as->ptr = updir;
  as->area = USER_SPACE;
  as->pgsize = PGSIZE;
  // map kernel space
  memcpy(updir, kas.ptr, PGSIZE);
}

void unprotect(AddrSpace *as) {
}

void __am_get_cur_as(Context *c) {
  c->pdir = (vme_enable ? (void *)get_satp() : NULL);
}

void __am_switch(Context *c) {
  if (vme_enable && c->pdir != NULL) {
    set_satp(c->pdir);
    // 必须加这两行！
    // 1. 刷新页表缓存 (虽然你可能没做 TLB，但 NEMU 内部有)
    //asm volatile("sfence.vma");
    
    // 2. 刷新指令缓存 (这是给 NEMU 模拟器看的！)
    // 告诉 NEMU："内存变了，把你缓存的那些旧指令扔掉，重新读！"
    //asm volatile("fence.i");
    printf("sfence.vma executed. fence.i executed\n");
  }
}

void map(AddrSpace *as, void *va, void *pa, int prot) {
  // 1. 获取一级页表基地址
  // as->ptr 存放的就是一级页表的物理地址
  PTE *pgdir = (PTE *)as->ptr;
  
  // 2. 获取一级页号 (VPN1) 和 二级页号 (VPN0)
  int vpn1 = PDX(va);
  int vpn0 = PTX(va);

  // 3. 检查一级页表项
  PTE *pde = &pgdir[vpn1]; // 指向一级页表中的第 vpn1 项
  
  if (!(*pde & PTE_V)) {
    // 如果一级页表项不存在 (Valid=0)
    // A. 申请一页新的物理页作为二级页表
    // pgalloc_usr 是 vme_init 传进来的回调函数，申请 4KB
    PTE *new_page = (PTE *)pgalloc_usr(PGSIZE); 
    
    // B. 新申请的页表必须清零！否则里面是垃圾数据，会导致乱映射
    // (注意：这里直接用 new_page 指针操作，因为在 AM 中 PA=VA，可以直接访问)
    memset(new_page, 0, PGSIZE);

    // C. 填写一级页表项
    // 格式：PPN | V
    // PPN 是物理地址右移 12 位，所以我们要把 new_page 地址处理一下
    // 这里暂时不管 R/W/X 权限，设为 V 即可，硬件会知道这是指向下一级的指针
    *pde = (((uintptr_t)new_page >> 12) << 10) | PTE_V;
  }

  // 4. 获取二级页表基地址
  // 从一级页表项中取出 PPN，还原成物理地址
  PTE *pgtable = (PTE *)PPN(*pde);

  // 5. 填写二级页表项
  PTE *pte = &pgtable[vpn0];
  
  uintptr_t pte_flags = PTE_V | PTE_R | PTE_W | PTE_X | PTE_A | PTE_D;
  // 用户段加上 PTE_U
  if ((uintptr_t)va < 0x80000000) {
      pte_flags |= PTE_U;
  }
  *pte = (((uintptr_t)pa >> 12) << 10) | pte_flags;
}

Context *ucontext(AddrSpace *as, Area kstack, void *entry) {
  // 1. [档案柜] 在内核栈里找个地方存“人设”(Context)
  Context *c = (Context*)kstack.end - 1;
  memset(c, 0, sizeof(Context));
  // 2. [VR眼镜] 绑定页表，醒来后看到的是在这个虚拟空间
  c->pdir = as->ptr;

  // 3. [大脑] 设定醒来后第一条指令执行的位置 (入口地址)
  c->mepc = (uintptr_t)entry;

  // 4. [身份] 设定醒来后的状态 (Machine Mode, 开启中断)
  c->mstatus = 0x80;

  // 5. [背包] 设定醒来后手里的栈指针 (指向用户栈底) <--- 你的问题3
  c->gpr[2] = (uintptr_t)as->area.end; 
  
  // 至于参数(问题2)，那是 Loader 之后往背包(用户栈)里塞东西的事，这里不管。

  return c;
}

/*
先暂停写代码，之前第一部分做的mmu部分是查找时vaddr到paddr，现在第二部分做的这个是将建立虚拟地址到物理地址的映射，一个读书一个写书。现在的问题是：1、什么时候分配？nano-lite可以看成一个简单的操作系统，nano-lite中main函数的运行看起来就是操作系统的启动，里面的
void init_proc() {
  switch_boot_pcb();
  Log("Initializing processes...");
  char *argv[] = {"/bin/busybox", "echo", "Hello", "Busybox", "from", "Nanos-lite", NULL};
  context_uload(&pcb[0], "/bin/busybox", argv);
  context_kload(&pcb[1], hello_fun, "B");
}
  应该就是操作系统创建用户态进程，也就是说实际上就是在这里需要进行map，这么理解对吗？第二、进程应该分为两个部分，数据段和代码段，两者应该是分开的，具体是这么操作的？补充
  void context_uload(PCB *pcb, const char *filename, char *const argv[]) {
  Log("Sizeof(Context) = %d", sizeof(Context));

  // 1. 加载程序，获取入口地址
  uintptr_t entry = loader(pcb, filename);

  // 2. 初始化栈指针到栈底 (高地址)
  // 我们使用 PCB 的内核栈区域作为用户栈 (PA4简化处理)
  void *sp = (void *)pcb->stack + STACK_SIZE;

  // 3. 处理参数 (User Process Arguments)
  // 目标栈布局 (从高地址向低地址生长):
  // [ String Area (参数字符串) ]
  // [ argv[n] = NULL          ]
  // [ ...                     ]
  // [ argv[0]                 ]
  // [ argc                    ]  <-- sp 指向这里

  int argc = 0;
  if (argv) {
    while (argv[argc]) argc++; // 统计参数个数
  }
  // ============ 新增调试 Log ============
  Log("context_uload: Loading file '%s', calculated argc = %d", filename, argc);
  // ====================================
  // 定义一个临时数组来存储字符串在栈上的新地址
  // (使用 uintptr_t 保证兼容 32/64 位)
  uintptr_t new_argv[argc > 0 ? argc : 1];
  // A. 拷贝字符串到栈上 (String Area)
  if (argc > 0) {
    for (int i = 0; i < argc; i++) {
      size_t len = strlen(argv[i]) + 1; // +1 for '\0'
      sp -= len; // 栈生长
      strcpy((char *)sp, argv[i]); // 拷贝字符串
      new_argv[i] = (uintptr_t)sp; // 记录新地址
    }
  }

  // B. 内存对齐 (Align)
  // 保持指针数组按字长对齐 (4字节 for 32位, 8字节 for 64位)
  //sp = (void *)((uintptr_t)sp & ~(sizeof(uintptr_t) - 1));
  sp = (void *)((uintptr_t)sp & ~0xF);
  // C. 填充 argv 指针数组
  // C-1. 结尾的 NULL
  sp -= sizeof(uintptr_t);
  *(uintptr_t *)sp = 0;

  // C-2. 依次填充 argv[i] (倒序入栈，这样 argv[0] 在低地址)
  if (argc > 0) {
    for (int i = argc - 1; i >= 0; i--) {
      sp -= sizeof(uintptr_t);
      *(uintptr_t *)sp = new_argv[i];
      printf("this the %d argv in %x\n", i, sp);
    }
  }
  // D. 填充 argc
  sp -= sizeof(uintptr_t);
  *(uintptr_t *)sp = argc;

  // 4. 调用 kcontext 创建上下文
  Area kstack;
  kstack.start = (void *)pcb;
  kstack.end = sp; 

  // ============ 关键修改 ============
  // 直接把 argc 作为第三个参数传进去！
  // kcontext 会自动把它放入 a0 寄存器，比我们要靠谱。
  pcb->cp = kcontext(kstack, (void*)entry, (void*)(uintptr_t)argc); 
  // ================================
  // 我们只需要手动处理 a1 (argv) 即可
  // argv 的地址就在 argc 的上面 (即 sp + 指针大小)
  pcb->cp->gpr[11] = (uintptr_t)sp + sizeof(uintptr_t); 
  pcb->cp->gpr[12] = 0;
  // 添加一条调试日志，看看 context 里到底存了啥
  Log("Context created: a0(argc)=%d, a1(argv)=%p, sp=%p", 
      pcb->cp->GPRx, pcb->cp->gpr[11], sp);

}
第三：除了上面两个地方需要map，别的还有吗？最后简单讲解map的规则
*/