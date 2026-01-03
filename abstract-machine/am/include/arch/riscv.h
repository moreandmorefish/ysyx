#ifndef ARCH_H__
#define ARCH_H__

// 1. 根据编译器宏定义确定寄存器数量
#ifdef __riscv_e
#define NR_REGS 16
#else
#define NR_REGS 32
#endif

// 2. 定义上下文结构体
struct Context {
  // [关键修复] 这里必须使用 NR_REGS，而不能写死 32
  // 对于 riscv32e，它是 16；对于标准 riscv32，它是 32。
  // 这必须与 trap.S 中的压栈顺序和数量完全一致！
  uintptr_t gpr[NR_REGS]; 
  
  uintptr_t mcause, mstatus, mepc;
  void *pdir;
  //uintptr_t np;
};

// 3. 定义关键寄存器别名 (用于 syscall 或传参)
#ifdef __riscv_e
#define GPR1 gpr[15] // a5 (x15) - riscv32e 的系统调用号/返回值寄存器
#else
#define GPR1 gpr[17] // a7 (x17) - 标准 riscv32 的系统调用号寄存器
#endif

// 定义参数寄存器 (RVE 和 RVI 的 a0-a2 都是 x10-x12)
#define GPR2 gpr[10] // a0
#define GPR3 gpr[11] // a1
#define GPR4 gpr[12] // a2
#define GPRx gpr[10] // a0 (返回值)

#endif