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
#include <memory/vaddr.h>
#include <memory/paddr.h>

// === 辅助宏 ===
#define VPN1(va) (((va) >> 22) & 0x3ff)
#define VPN0(va) (((va) >> 12) & 0x3ff)
#define OFF(va)  ((va) & 0xfff)
#define PTE_PPN(pte) (((pte) >> 10) << 12)
#define PTE_V 0x01
#define SATP_MODE_SV32 0x80000000

// === 检查是否需要翻译 ===
int isa_mmu_check(vaddr_t vaddr, int len, int type) {
  // 如果 satp 的 MODE 位为 1，则开启翻译
  if (cpu.csr.satp & SATP_MODE_SV32) {
    return MMU_TRANSLATE;
  }
  return MMU_DIRECT;
}

// === 地址翻译核心逻辑 ===
paddr_t isa_mmu_translate(vaddr_t vaddr, int len, int type) {
  // Step 1: 获取根页表物理基址 (从 satp)
  paddr_t pg_dir_base = (cpu.csr.satp & 0x3fffff) << 12;

  // Step 2: 查一级页表
  paddr_t pte1_addr = pg_dir_base + (VPN1(vaddr) * 4);
  word_t pte1 = paddr_read(pte1_addr, 4);

  if (!(pte1 & PTE_V)) {
    if (vaddr < 0x1000) {
       printf("MMU L1 Fail: vaddr=%x, satp=%x, dir_base=%x, vpn1=%d\n", 
              vaddr, cpu.csr.satp, (cpu.csr.satp & 0x3fffff) << 12, vaddr >> 22);
    }
    // 遇到缺页或非法映射，PA4阶段建议直接报错，方便定位
    panic("MMU Translation Failed at L1: vaddr = 0x%x, pte1_addr = 0x%x", vaddr, pte1_addr);
  }

  // Step 3: 查二级页表
  paddr_t pg_table_base = PTE_PPN(pte1);
  paddr_t pte2_addr = pg_table_base + (VPN0(vaddr) * 4);
  word_t pte2 = paddr_read(pte2_addr, 4);

  if (!(pte2 & PTE_V)) {
    panic("MMU Translation Failed at L2: vaddr = 0x%x, pte2_addr = 0x%x", vaddr, pte2_addr);
  }

  // Step 4: 拼接物理地址
  return PTE_PPN(pte2) | OFF(vaddr);
}
