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

Context* kcontext(Area kstack, void (*entry)(void *), void *arg);

void context_uload(PCB *pcb, const char *filename, char *const argv[]) {

  Log("Sizeof(Context) = %d", sizeof(Context));
  
  // 1. 加载程序，获取入口地址
  uintptr_t entry = loader(pcb, filename);

  // 2. 初始化栈指针到栈底 (高地址)
  // 我们使用 PCB 的内核栈区域作为用户栈 (PA4简化处理)
  void *sp = (void *)pcb->stack + STACK_SIZE;

  // 3. 处理参数 (User Process Arguments)
  // 目标栈布局 (从高地址向低地址生长):
  // [ String Area (参数字符串) ]
  // [ argv[n] = NULL          ]
  // [ ...                     ]
  // [ argv[0]                 ]
  // [ argc                    ]  <-- sp 指向这里

  int argc = 0;
  if (argv) {
    while (argv[argc]) argc++; // 统计参数个数
  }

  // ============ 新增调试 Log ============
  Log("context_uload: Loading file '%s', calculated argc = %d", filename, argc);
  // ====================================

  // 定义一个临时数组来存储字符串在栈上的新地址
  // (使用 uintptr_t 保证兼容 32/64 位)
  uintptr_t new_argv[argc > 0 ? argc : 1];

  // A. 拷贝字符串到栈上 (String Area)
  if (argc > 0) {
    for (int i = 0; i < argc; i++) {
      size_t len = strlen(argv[i]) + 1; // +1 for '\0'
      sp -= len; // 栈生长
      strcpy((char *)sp, argv[i]); // 拷贝字符串
      new_argv[i] = (uintptr_t)sp; // 记录新地址
    }
  }

  // B. 内存对齐 (Align)
  // 保持指针数组按字长对齐 (4字节 for 32位, 8字节 for 64位)
  //sp = (void *)((uintptr_t)sp & ~(sizeof(uintptr_t) - 1));
  sp = (void *)((uintptr_t)sp & ~0xF);
  // C. 填充 argv 指针数组
  // C-1. 结尾的 NULL
  sp -= sizeof(uintptr_t);
  *(uintptr_t *)sp = 0;

  // C-2. 依次填充 argv[i] (倒序入栈，这样 argv[0] 在低地址)
  if (argc > 0) {
    for (int i = argc - 1; i >= 0; i--) {
      sp -= sizeof(uintptr_t);
      *(uintptr_t *)sp = new_argv[i];
      printf("this the %d argv in %x\n", i, sp);
    }
  }

  // D. 填充 argc
  sp -= sizeof(uintptr_t);
  *(uintptr_t *)sp = argc;

  // 4. 调用 kcontext 创建上下文
  Area kstack;
  kstack.start = (void *)pcb;
  kstack.end = sp; 

  // ============ 关键修改 ============
  // 直接把 argc 作为第三个参数传进去！
  // kcontext 会自动把它放入 a0 寄存器，比我们要靠谱。
  pcb->cp = kcontext(kstack, (void*)entry, (void*)(uintptr_t)argc); 
  // ================================

  // 我们只需要手动处理 a1 (argv) 即可
  // argv 的地址就在 argc 的上面 (即 sp + 指针大小)
  pcb->cp->gpr[11] = (uintptr_t)sp + sizeof(uintptr_t); 

  // 添加一条调试日志，看看 context 里到底存了啥
  Log("Context created: a0(argc)=%d, a1(argv)=%p, sp=%p", 
      pcb->cp->GPRx, pcb->cp->gpr[11], sp);
}