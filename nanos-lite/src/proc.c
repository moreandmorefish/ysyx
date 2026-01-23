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
    // 每隔 100次调度 打印一次，防止刷屏太快
    if (j % 10 == 0) {
      Log("Hello World from Nanos-lite with arg '%s' for the %dth time!", (char*)arg, j);
    }
    j ++;
    yield();
  }
}

// ================= 1. 实现 context_kload =================
void context_kload(PCB *pcb, void (*entry)(void *), void *arg) {
  Area kstack;
  // 核心逻辑：计算栈的范围
  // PCB 结构体位于栈底 (低地址)，栈顶在 高地址
  kstack.start = (void*)pcb->stack;
  kstack.end = (void*)pcb->stack + STACK_SIZE;

  // 调用 AM 的 kcontext 构造上下文，并保存在 PCB 中
  pcb->cp = kcontext(kstack, entry, arg);
}

void init_proc() {
  switch_boot_pcb();

  Log("Initializing processes...");

  // ================= 2. 修改初始化逻辑 =================
  
  //char *argv[] = {"/bin/busybox", "echo", "Hello", "Busybox", "from", "Nanos-lite", NULL};
  context_uload(&pcb[0], "/bin/hello", NULL);
  context_uload(&pcb[1], "/bin/pal", NULL);
  

  // 暂时注释掉加载用户程序，先测试内核线程切换
  // naive_uload(NULL, "/bin/menu");
}
void check_dummy_entry_permission(PCB *pcb);

Context* schedule(Context *prev) {
  current->cp = prev;
  current->cp->pdir = current->as.ptr;
  current = (current == &pcb[0] ? &pcb[1] : &pcb[0]);

  // ================= 实验探针 =================
  // 我们只盯着 dummy (pcb[1]) 看
  if (current == &pcb[1]) {
    // 打印出 dummy 此时此刻持有的一级页表物理地址 (pdir)
    Log("DEBUG CHECK: Dummy is returning. cp=%x, cp->pdir=%x, as.ptr=%x", 
        current->cp, current->cp->pdir, current->as.ptr);
    
    // 逻辑验证：如果 pdir 是 0，或者是一个奇怪的值（比如 ASCII 码），那就出大问题了
    if (current->cp->pdir != current->as.ptr) {
      Log("!!! ALARM !!! Stack Context pdir (garbage?) does NOT match Process pdir!");
    }
  }
  // ===========================================

  if (current == &pcb[0]) {
      // 保持你原来的 Log 不动
      Log("DEBUG: Hello_fun gp register = %x", current->cp->gpr[3]);
  }
  else Log("DEBUG: dummy gp register = %x", current->cp->gpr[3]);
  
  __am_switch(current->cp);
  return current->cp;
}
// Context* schedule(Context *prev) {
//   // 1. 保存当前上下文
//   current->cp = prev;
// 
//   // 2. [关键修复] 强制修复栈上的 pdir
//   // 即使栈被踩了，这行也能把正确的页表地址写回去！
//   current->cp->pdir = current->as.ptr;
// 
//   // 3. 切换进程
//   current = (current == &pcb[0] ? &pcb[1] : &pcb[0]);
// 
//   // 4. 切换页表
//   __am_switch(current->cp);
// 
//   return current->cp;
// }


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