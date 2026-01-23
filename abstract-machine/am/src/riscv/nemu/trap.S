#define concat_temp(x, y) x ## y
#define concat(x, y) concat_temp(x, y)
#define MAP(c, f) c(f)

#if __riscv_xlen == 32
#define LOAD  lw
#define STORE sw
#define XLEN  4
#else
#define LOAD  ld
#define STORE sd
#define XLEN  8
#endif

#define REGS_LO16(f) \
      f( 1)       f( 3) f( 4) f( 5) f( 6) f( 7) f( 8) f( 9) \
f(10) f(11) f(12) f(13) f(14) f(15)
#ifndef __riscv_e
#define REGS_HI16(f) \
                                    f(16) f(17) f(18) f(19) \
f(20) f(21) f(22) f(23) f(24) f(25) f(26) f(27) f(28) f(29) \
f(30) f(31)
#define NR_REGS 32
#else
#define REGS_HI16(f)
#define NR_REGS 16
#endif

#define REGS(f) REGS_LO16(f) REGS_HI16(f)

#define PUSH(n) STORE concat(x, n), (n * XLEN)(sp);
#define POP(n)  LOAD  concat(x, n), (n * XLEN)(sp);

// [修改] 增加栈大小以容纳 np
// Context 大小 = (32个通用寄存器 + mcause + mstatus + mepc + pdir + np) * 4
#define CONTEXT_SIZE  ((NR_REGS + 5) * XLEN)

#define OFFSET_SP     ( 2 * XLEN)
#define OFFSET_CAUSE  ((NR_REGS + 0) * XLEN)
#define OFFSET_STATUS ((NR_REGS + 1) * XLEN)
#define OFFSET_EPC    ((NR_REGS + 2) * XLEN)
// OFFSET_PDIR 在 +3 的位置
#define OFFSET_NP     ((NR_REGS + 4) * XLEN) // [新增] np 的偏移量

// [新增] 特权级宏定义 (必须与 C 代码一致)
#define KERNEL_MODE 3
#define USER_MODE   0

.align 3
.globl __am_asm_trap
__am_asm_trap:
  // ================= 核心魔法开始 =================
  // 1. 原子交换 sp 和 mscratch
  //    - 如果从用户态来: mscratch 存的是内核栈顶。交换后 sp=内核栈, mscratch=用户栈。
  //    - 如果从内核态来: mscratch 存的是 0。交换后 sp=0, mscratch=内核栈(原sp)。
  csrrw sp, mscratch, sp

  // 2. 判断来源 (通过检查现在的 sp)
  bnez sp, save_context_user   // 如果 sp != 0，说明是从用户态来的 (交换成功)

save_context_kernel:
  // [从内核态重入]
  // 刚才交换导致 sp 变成 0 了，原本的 sp 在 mscratch 里。
  csrr sp, mscratch            // 把原来的内核栈指针读回来
  
  addi sp, sp, -CONTEXT_SIZE   // 开辟栈空间
  
  // 保存 np = KERNEL_MODE
  li t0, KERNEL_MODE
  STORE t0, OFFSET_NP(sp)
  
  // 保存 sp (这里的 sp 就是发生中断时的栈指针)
  // 此时 sp 已经减过了，所以要加回去才是“原来的 sp”
  addi t0, sp, CONTEXT_SIZE
  STORE t0, OFFSET_SP(sp)
  
  j save_common                // 跳过用户态处理逻辑

save_context_user:
  // [从用户态陷入]
  // 此时 sp 已经是内核栈了。mscratch 里存的是原来的 User SP。
  
  addi sp, sp, -CONTEXT_SIZE   // 开辟栈空间
  
  // 保存 np = USER_MODE
  li t0, USER_MODE
  STORE t0, OFFSET_NP(sp)
  
  // 保存 User SP (从 mscratch 读出来，存到 c->sp)
  csrr t0, mscratch
  STORE t0, OFFSET_SP(sp)
  
  // [重要] 进入内核后，必须将 mscratch 置 0
  // 这样下次如果发生嵌套中断，就能识别出是内核态重入
  csrw mscratch, zero

save_common:
  // ================= 核心魔法结束 =================

  // 3. 保存通用寄存器 (x1, x3-x31) -- 注意宏 REGS 不包含 x0 和 x2(sp)
  MAP(REGS, PUSH)

  // 4. 保存 CSR
  csrr t0, mcause
  csrr t1, mstatus
  csrr t2, mepc
  STORE t0, OFFSET_CAUSE(sp)
  STORE t1, OFFSET_STATUS(sp)
  STORE t2, OFFSET_EPC(sp)

  // 5. 必须禁用 MPRV (防止内核访问自己的数据时被当做用户态拦截)
  // 如果之前为了 difftest 开启了它，现在必须关闭
  // li a0, (1 << 17)
  // not a0, a0
  // and t1, t1, a0
  // csrw mstatus, t1

  // 6. 调用 C 语言处理函数
  mv a0, sp
  call __am_irq_handle
  
  // 7. 切换栈指针 (a0 返回的是新进程的 Context 指针，也就是新进程的内核栈顶)
  mv sp, a0

  // ================= 恢复逻辑开始 =================
  
  // 8. 恢复 CSR
  LOAD t1, OFFSET_STATUS(sp)
  LOAD t2, OFFSET_EPC(sp)
  csrw mstatus, t1
  csrw mepc, t2

  // 9. 决定去向 (检查 c->np)
  LOAD t0, OFFSET_NP(sp)
  li t1, USER_MODE
  beq t0, t1, restore_user     // 如果 np == USER，跳转处理

restore_kernel:
  // [回内核态]
  // 不需要设置 mscratch (保持为 0)
  // 不需要恢复 sp (因为我们就在内核栈上，直接 POP 就行)
  // 但是！Context 里的 sp 是“保存时的值(高地址)”，而我们现在栈指针已经在低地址了
  // 所以直接走下面的 POP 流程自然会恢复栈平衡
  j restore_common

restore_user:
  // [回用户态]
  // 1. 设置 mscratch = 内核栈顶 (以便下次 Trap 使用)
  //    此时 sp 指向 Context 底部。
  //    内核栈顶 = sp + CONTEXT_SIZE
  addi t1, sp, CONTEXT_SIZE
  csrw mscratch, t1
  
  // 2. 准备恢复 User SP
  //    稍后在 restore_common 里，LOAD sp, OFFSET_SP(sp) 会覆盖当前的内核栈指针
  
restore_common:
  // 10. 恢复通用寄存器
  MAP(REGS, POP)

  // 11. 恢复 SP (关键!)
  // 对于用户进程，这行指令会从 c->sp 读取 User Stack 地址，写入 sp 寄存器
  // 执行完这句，我们正式离开了内核栈，踩在了用户栈上
  LOAD sp, OFFSET_SP(sp)
  
  // 12. 返回
  mret