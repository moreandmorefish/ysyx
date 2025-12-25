#include <proc.h>

#define MAX_NR_PROC 4

static PCB pcb[MAX_NR_PROC] __attribute__((used)) = {};
static PCB pcb_boot = {};
PCB *current = NULL;

void naive_uload(PCB *pcb, const char *filename);

// 声明外部函数 kcontext (如果头文件包含不全，这行可以防止编译警告)
Context* kcontext(Area kstack, void (*entry)(void *), void *arg);
void context_uload(PCB *pcb, const char *filename, char *const argv[]);

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
  kstack.start = (void*)pcb;
  kstack.end = (void*)pcb + STACK_SIZE;

  // 调用 AM 的 kcontext 构造上下文，并保存在 PCB 中
  pcb->cp = kcontext(kstack, entry, arg);
}

void init_proc() {
  switch_boot_pcb();

  Log("Initializing processes...");

  // ================= 2. 修改初始化逻辑 =================
  
  char *argv[] = {"/bin/dummy", "--skip", NULL};

  context_uload(&pcb[0], "/bin/dummy", argv);
  context_kload(&pcb[1], hello_fun, "B");

  // 暂时注释掉加载用户程序，先测试内核线程切换
  // naive_uload(NULL, "/bin/menu");
}

Context* schedule(Context *prev) {
  // ================= 3. 实现调度逻辑 =================
  
  // 1. 保存当前进程的上下文
  // 这里的 prev 是发生中断/yield 前那个进程的 Context 指针
  current->cp = prev;

  // 2. 选择下一个要运行的进程 (简单的轮转调度)
  // 如果当前是 pcb[0]，就切到 pcb[1]；否则切回 pcb[0]
  // 注意：如果是从 boot (main) 第一次进来，current 是 pcb_boot，这里也会切到 pcb[0]
  current = (current == &pcb[0] ? &pcb[1] : &pcb[0]);

  // 3. 返回新进程的上下文指针
  // trap.S 里的汇编代码会把这个指针赋值给 sp，然后恢复寄存器
  return current->cp;
}