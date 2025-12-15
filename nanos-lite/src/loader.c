#include <proc.h>
#include <elf.h>
#include <fs.h> // 确保包含了 fs_open 等声明

#ifdef __LP64__
# define Elf_Ehdr Elf64_Ehdr
# define Elf_Phdr Elf64_Phdr
#else
# define Elf_Ehdr Elf32_Ehdr
# define Elf_Phdr Elf32_Phdr
#endif

static uintptr_t loader(PCB *pcb, const char *filename) {
  // 1. 打开文件
  int fd = fs_open(filename, 0, 0); // flags 和 mode 在 SFS 中被忽略

  // 2. 读取 ELF Header
  Elf_Ehdr ehdr;
  fs_read(fd, &ehdr, sizeof(Elf_Ehdr));

  // 检查魔数
  assert(*(uint32_t *)ehdr.e_ident == 0x464c457f);

  // 3. 遍历 Program Headers
  Elf_Phdr phdr;
  for (int i = 0; i < ehdr.e_phnum; i++) {
    // 定位到当前 Program Header 的位置
    // e_phoff 是 PHT 在文件中的起始偏移
    fs_lseek(fd, ehdr.e_phoff + i * ehdr.e_phentsize, SEEK_SET);
    // 读取这个 Header
    fs_read(fd, &phdr, sizeof(Elf_Phdr));

    if (phdr.p_type == PT_LOAD) {
      Log("Loading segment %d: p_vaddr=%p, p_offset=%p, p_filesz=%u, p_memsz=%u", 
          i, phdr.p_vaddr, phdr.p_offset, phdr.p_filesz, phdr.p_memsz);
          
      // 定位到 Segment 在文件中的偏移
      fs_lseek(fd, phdr.p_offset, SEEK_SET);
      
      // 读取数据到内存
      fs_read(fd, (void *)phdr.p_vaddr, phdr.p_filesz);

      // 清零 BSS
      size_t bss_size = phdr.p_memsz - phdr.p_filesz;
      if (bss_size > 0) {
        memset((void *)(phdr.p_vaddr + phdr.p_filesz), 0, bss_size);
      }
    }
  }

  // 4. 关闭文件
  fs_close(fd);

  return ehdr.e_entry;
}

void naive_uload(PCB *pcb, const char *filename) {
  uintptr_t entry = loader(pcb, filename);
  Log("Jump to entry = %p", entry);
  ((void(*)())entry) ();
}