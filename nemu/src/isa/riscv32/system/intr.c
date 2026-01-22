/***************************************************************************************
* Copyright (c) 2014-2024 Zihao Yu, Nanjing University
*
* NEMU is licensed under Mulan PSL v2.
* You can use this software according to the terms and conditions of the Mulan PSL v2.
* You may obtain a copy of Mulan PSL v2 at:
*          http://license.coscl.org.cn/MulanPSL2
*
* THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
* EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
* MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
*
* See the Mulan PSL v2 for more details.
***************************************************************************************/

#include <isa.h>

#include <isa.h>

// 响应中断：硬件自动保存现场并跳转
word_t isa_raise_intr(word_t NO, vaddr_t epc) {
  /* 1. 保存当前 PC 到 mepc */
  cpu.csr.mepc = epc;

  /* 2. 保存中断原因到 mcause */
  cpu.csr.mcause = NO;

  /* 3. 硬件原子操作：保存并关闭中断 (Critical!) */
  // a. 将 MIE (Bit 3) 保存到 MPIE (Bit 7)
  if (cpu.csr.mstatus & MSTATUS_MIE) {
    cpu.csr.mstatus |= MSTATUS_MPIE;
  } else {
    cpu.csr.mstatus &= ~MSTATUS_MPIE;
  }
  
  // b. 关闭 MIE (Bit 3) -> 关中断
  cpu.csr.mstatus &= ~MSTATUS_MIE;

  /* 4. 返回入口地址 mtvec */
  // (通常 RISC-V 还有 Mode 判断，PA 中简化为直接跳 mtvec)
  return cpu.csr.mtvec;
}

// 查询中断：CPU 检查是否要处理中断
word_t isa_query_intr() {
  // 条件：引脚被拉高 (INTR) 且 没带耳机 (MIE=1)
  if (cpu.INTR && (cpu.csr.mstatus & MSTATUS_MIE)) {
    cpu.INTR = false; // 响应后拉低引脚 (边缘触发模拟)
    return IRQ_TIMER; // 返回时钟中断号 0x80000007
  }
  
  return INTR_EMPTY;
}
