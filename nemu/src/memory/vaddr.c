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
#include <memory/paddr.h>

// 取指令
word_t vaddr_ifetch(vaddr_t addr, int len) {
  // 1. 检查是否开启分页 (MEM_TYPE_IFETCH)
  int ret = isa_mmu_check(addr, len, MEM_TYPE_IFETCH);
  
  // 2. 如果是 MMU_DIRECT (satp=0 或 M模式)，直接读物理内存
  if (ret == MMU_DIRECT) {
    return paddr_read(addr, len);
  } 
  // 3. 否则进行翻译 (MMU_TRANSLATE)
  else {
    paddr_t paddr = isa_mmu_translate(addr, len, MEM_TYPE_IFETCH);
    return paddr_read(paddr, len);
  }
}

// 读数据
word_t vaddr_read(vaddr_t addr, int len) {
  int ret = isa_mmu_check(addr, len, MEM_TYPE_READ);
  if (ret == MMU_DIRECT) {
    return paddr_read(addr, len);
  } else {
    paddr_t paddr = isa_mmu_translate(addr, len, MEM_TYPE_READ);
    return paddr_read(paddr, len);
  }
}

// 写数据
void vaddr_write(vaddr_t addr, int len, word_t data) {
  int ret = isa_mmu_check(addr, len, MEM_TYPE_WRITE);
  if (ret == MMU_DIRECT) {
    paddr_write(addr, len, data);
  } else {
    paddr_t paddr = isa_mmu_translate(addr, len, MEM_TYPE_WRITE);
    paddr_write(paddr, len, data);
  }
}
