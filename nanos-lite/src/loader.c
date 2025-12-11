#include <proc.h>
#include <elf.h>
#include <fs.h> // 需要包含 fs.h 或 ramdisk.h 来获取 ramdisk_read

#ifdef __LP64__
# define Elf_Ehdr Elf64_Ehdr
# define Elf_Phdr Elf64_Phdr
#else
# define Elf_Ehdr Elf32_Ehdr
# define Elf_Phdr Elf32_Phdr
#endif

// ramdisk 的起始偏移为 0
#define RAMDISK_START_OFFSET 0 

static uintptr_t loader(PCB *pcb, const char *filename) {
  // 1. 读取 ELF 文件头
  Elf_Ehdr ehdr;
  ramdisk_read(&ehdr, RAMDISK_START_OFFSET, sizeof(Elf_Ehdr));
  
  // 检查魔数
  // Magic Number: 0x7f 'E' 'L' 'F' (对应 0x464c457f, 字节序可能不同)
  // RISCV32 是小端序
  assert(*(uint32_t *)ehdr.e_ident == 0x464c457f); // 正确的 ELF 魔数
  // ... 检查 ISA 类型 (可选，推荐)
  
  // 2. 定位 Program Header Table
  // Ehdr 的 e_phoff 记录了 PHT 相对于文件开头的偏移
  // Ehdr 的 e_phnum 记录了 PHT 中条目数量
  Elf_Phdr phdr; 
  
  // 3. 遍历 Program Header Table
  for (int i = 0; i < ehdr.e_phnum; i++) {
    // 计算当前 Program Header 在 ramdisk 中的实际偏移
    size_t phdr_offset = ehdr.e_phoff + i * ehdr.e_phentsize;
    
    // 从 ramdisk 读取当前的 Program Header
    ramdisk_read(&phdr, phdr_offset, sizeof(Elf_Phdr));
    
    // 4. 判断是否需要加载
    if (phdr.p_type == PT_LOAD) {
      Log("Loading segment %d: p_vaddr=%p, p_offset=%p, p_filesz=%u, p_memsz=%u", 
          i, phdr.p_vaddr, phdr.p_offset, phdr.p_filesz, phdr.p_memsz);
          
      // A. 从 ramdisk 复制数据到内存 (p_filesz 部分)
      // 目标地址: phdr.p_vaddr
      // 源偏移: phdr.p_offset
      // 长度: phdr.p_filesz
      ramdisk_read((void *)phdr.p_vaddr, phdr.p_offset, phdr.p_filesz);
      
      // B. 清零 BSS 段 (p_memsz - p_filesz 部分)
      // BSS 段起始地址: phdr.p_vaddr + phdr.p_filesz
      // 长度: phdr.p_memsz - phdr.p_filesz
      size_t bss_size = phdr.p_memsz - phdr.p_filesz;
      if (bss_size > 0) {
        // 使用 memset 清零
        memset((void *)(phdr.p_vaddr + phdr.p_filesz), 0, bss_size);
      }
    }
  }
  
  // 5. 返回程序入口地址
  return ehdr.e_entry;
}

void naive_uload(PCB *pcb, const char *filename) {
  uintptr_t entry = loader(pcb, filename);
  Log("Jump to entry = %p", entry);
  ((void(*)())entry) ();
}

