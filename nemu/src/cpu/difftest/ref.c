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
#include <cpu/cpu.h>
#include <difftest-def.h>
#include <memory/paddr.h>

// 声明 NEMU 内部的 CPU 结构体，确保我们可以访问它
// 通常在 cpu/cpu.h 中定义了 CPU_state cpu;
extern CPU_state cpu;

// 1. 内存拷贝：让 DUT 初始化时把程序代码塞进 NEMU
__EXPORT void difftest_memcpy(paddr_t addr, void *buf, size_t n, bool direction) {
  // 此时只支持将数据写入 NEMU (DIFFTEST_TO_REF)
  if (direction == DIFFTEST_TO_REF) {
    // 逐字节写入物理内存
    // 使用 NEMU 提供的 paddr_write 接口，确保处理好内存映射
    for (size_t i = 0; i < n; i++) {
      paddr_write(addr + i, 1, ((uint8_t *)buf)[i]);
    }
  } else {
    assert(0); // 目前不需要从 NEMU 读出内存
  }
}

// 2. 寄存器拷贝：用于对比状态
__EXPORT void difftest_regcpy(void *dut, bool direction) {
  if (direction == DIFFTEST_TO_DUT) {
    // 【读出】：将 NEMU 的寄存器拷贝到外部提供的 buffer (dut) 中
    // 这里的 buffer 必须和 CPU_state 结构体大小布局一致
    memcpy(dut, &cpu, sizeof(cpu));
  } else {
    // 【写入】：强制覆盖 NEMU 的寄存器 (通常用于初始化同步)
    memcpy(&cpu, dut, sizeof(cpu));
  }
}

// 3. 执行指令：让 NEMU 跑 n 步
__EXPORT void difftest_exec(uint64_t n) {
  cpu_exec(n);
}

// 4. 中断触发 (暂时不用，留空即可)
__EXPORT void difftest_raise_intr(word_t NO) {
  assert(0);
}

// 5. 初始化
__EXPORT void difftest_init(int port) {
  // 只需要初始化内存，ISA 初始化通常在 cpu_exec 前自动处理或在此处调用
  void init_mem();
  init_mem();
  
  // 初始化寄存器为 0，避免随机值干扰
  memset(&cpu, 0, sizeof(cpu));
  cpu.pc = 0x80000000; // 默认复位 PC，根据你的 linker.ld 调整
  
  init_isa();
}
