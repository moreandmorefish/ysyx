.globl  _start
_start:

#if defined(__ISA_AM_NATIVE__)

  movq $0, %rbp
  // (rsp + 8) should be multiple of 16 when
  // control is transfered to the function entry point.
  // See amd64 ABI manual for more details
  andq $0xfffffffffffffff0, %rsp
  movq %rax, %rdi
  call call_main

#elif defined(__ISA_X86_64__)

  movq $0, %rbp
  movq %rax, %rsp
  movq %rax, %rdi
  call call_main

#elif defined(__ISA_X86__)

  movl $0, %ebp
  call call_main

#elif defined(__ISA_MIPS32__)

  move $fp, $zero
  jal call_main

#elif defined(__riscv)
  # [重点修改] 强制将栈指针设为 0x87FFFFF0 (内存顶端)
  # 这样栈从 0x88000000 向下长，程序从 0x83000000 向上长，中间极其安全
  lui sp, 0x88000
  addi sp, sp, -16

  # 清空 a 寄存器，防止 main 函数误读 argv/envp 导致小鸟等程序崩溃
  li a0, 0
  li a1, 0
  li a2, 0
  
  mv s0, zero
  jal call_main

#elif defined(__ISA_LOONGARCH32R__)

  move $fp, $zero
  bl call_main

#else
#error Unsupported ISA
#endif
