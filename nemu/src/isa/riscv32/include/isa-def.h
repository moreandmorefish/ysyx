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

#ifndef __ISA_RISCV_H__
#define __ISA_RISCV_H__

#include <common.h>

// --- mstatus 寄存器的位掩码定义 ---
#define MSTATUS_MIE  (1 << 3)   // Machine Interrupt Enable
#define MSTATUS_MPIE (1 << 7)   // Machine Previous Interrupt Enable
#define MSTATUS_SPP  (1 << 8)   // Supervisor Previous Privilege
#define MSTATUS_MPP  (3 << 11)  // [新增] Machine Previous Privilege (Bits 11-12)

// --- 特权级模式定义 [新增] ---
enum { 
  U_MODE = 0, // User Mode
  S_MODE = 1, // Supervisor Mode
  M_MODE = 3  // Machine Mode
};

// --- 中断号定义 ---
#define IRQ_TIMER    0x80000007

typedef struct {
  word_t mcause;
  word_t mstatus;
  vaddr_t mepc;
  word_t mtvec;
  word_t satp;   
  word_t mscratch;
} riscv32_CSRs;

typedef struct {
  word_t gpr[MUXDEF(CONFIG_RVE, 16, 32)];
  vaddr_t pc;
  riscv32_CSRs csr;

  // --- 中断引脚状态 ---
  bool INTR;

  // --- [新增] 当前特权级 ---
  // CPU 当前运行在哪个模式 (U_MODE / M_MODE)
  uint8_t mode; 
  
} MUXDEF(CONFIG_RV64, riscv64_CPU_state, riscv32_CPU_state);

// decode
typedef struct {
  uint32_t inst;
} MUXDEF(CONFIG_RV64, riscv64_ISADecodeInfo, riscv32_ISADecodeInfo);

#endif
