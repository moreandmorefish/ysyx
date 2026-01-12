#include <proc.h>

#define MAX_NR_PROC 4

static PCB pcb[MAX_NR_PROC] __attribute__((used)) = {};
static PCB pcb_boot = {};
PCB *current = NULL;

void naive_uload(PCB *pcb, const char *filename);

// 声明外部函数 kcontext (如果头文件包含不全，这行可以防止编译警告)
Context* kcontext(Area kstack, void (*entry)(void *), void *arg);
void context_uload(PCB *pcb, const char *filename, char *const argv[]);
void __am_switch(Context *c);

void switch_boot_pcb() {
  current = &pcb_boot;
}

void hello_fun(void *arg) {
  int j = 1;
  while (1) {
    // 打印 arg，这样我们就能区分是哪个线程在跑了
    Log("Hello World from Nanos-lite with arg '%s' for the %dth time!", (char*)arg, j);
    j ++;
    yield(); // 主动让出 CPU，触发 schedule
  }
}

// ================= 1. 实现 context_kload =================
void context_kload(PCB *pcb, void (*entry)(void *), void *arg) {
  Area kstack;
  // 核心逻辑：计算栈的范围
  // PCB 结构体位于栈底 (低地址)，栈顶在 高地址
  kstack.start = (void*)pcb->stack;
  kstack.end = (void*)pcb + STACK_SIZE;

  // 调用 AM 的 kcontext 构造上下文，并保存在 PCB 中
  pcb->cp = kcontext(kstack, entry, arg);
}

void init_proc() {
  switch_boot_pcb();

  Log("Initializing processes...");

  // ================= 2. 修改初始化逻辑 =================
  
  //char *argv[] = {"/bin/busybox", "echo", "Hello", "Busybox", "from", "Nanos-lite", NULL};
  context_kload(&pcb[0], hello_fun, "B");
  context_uload(&pcb[1], "/bin/dummy", NULL);
  

  // 暂时注释掉加载用户程序，先测试内核线程切换
  // naive_uload(NULL, "/bin/menu");
}
void check_dummy_entry_permission(PCB *pcb);
Context* schedule(Context *prev) {
  // ================= 3. 实现调度逻辑 =================
  
  // 1. 保存当前进程的上下文
  // 这里的 prev 是发生中断/yield 前那个进程的 Context 指针
  current->cp = prev;

  // 2. 选择下一个要运行的进程 (简单的轮转调度)
  // 如果当前是 pcb[0]，就切到 pcb[1]；否则切回 pcb[0]
  // 注意：如果是从 boot (main) 第一次进来，current 是 pcb_boot，这里也会切到 pcb[0]
  current = (current == &pcb[0] ? &pcb[1] : &pcb[0]);
  Log("Switching to PCB[%d], cp=%x, sp=%x, mepc=%x", 
      (current == &pcb[0] ? 0 : 1), 
      current->cp, 
      current->cp->gpr[2],  // <--- 看看它是 0 吗？
      current->cp->mepc);

  if (current == &pcb[1]) { // 如果切到 dummy
    // 打印 mtvec 寄存器，确认异常入口是否正常
    uintptr_t mtvec;
    asm volatile("csrr %0, mtvec" : "=r"(mtvec));
    Log("Current mtvec = %x", mtvec);

    // 检查页表权限
    check_dummy_entry_permission(current);
  }
  
  __am_switch(current->cp);
  // 3. 返回新进程的上下文指针
  // trap.S 里的汇编代码会把这个指针赋值给 sp，然后恢复寄存器
  return current->cp;
}


// 在 schedule 函数前面定义
void check_dummy_entry_permission(PCB *pcb) {
  uintptr_t vaddr = 0x40004d48; // dummy 的入口地址 (根据你的Log)
  uintptr_t pdir_base = (uintptr_t)pcb->as.ptr; // 页表物理基地址

  // 1. 查一级页表 (L1)
  int vpn1 = vaddr >> 22;
  uintptr_t *pgdir = (uintptr_t *)pdir_base;
  uintptr_t pde = pgdir[vpn1];

  Log("Checking vaddr %x: L1 index=%d, PDE=%x", vaddr, vpn1, pde);

  if (!(pde & 0x1)) { // 检查 Valid 位
    Log("Error: L1 entry invalid!");
    return;
  }

  // 2. 查二级页表 (L2)
  int vpn0 = (vaddr >> 12) & 0x3ff;
  uintptr_t *pgtable = (uintptr_t *)((pde >> 10) << 12); // PPN -> PhysAddr
  uintptr_t pte = pgtable[vpn0];

  Log("Checking vaddr %x: L2 index=%d, PTE=%x", vaddr, vpn0, pte);

  // 3. 检查 PTE_U (User) 位，通常是第 4 位 (0x10)
  if (pte & 0x10) {
    Log("Pass: PTE_U bit is SET. User can access.");
  } else {
    Log("Fail: PTE_U bit is NOT SET! User cannot access -> Page Fault.");
  }
}