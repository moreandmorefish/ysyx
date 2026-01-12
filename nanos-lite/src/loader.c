#include <proc.h>
#include <elf.h>
#include <fs.h>

#ifdef __LP64__
# define Elf_Ehdr Elf64_Ehdr
# define Elf_Phdr Elf64_Phdr
#else
# define Elf_Ehdr Elf32_Ehdr
# define Elf_Phdr Elf32_Phdr
#endif

// 声明外部函数，如果头文件里没包含的话
extern void* new_page(size_t nr_page);

// 辅助宏
#define PGSIZE 4096
#define PTE_V 0x01
#define PGMASK (PGSIZE - 1)

static uintptr_t loader(PCB *pcb, const char *filename) {
  Log("Loader start: %s", filename);

  // 1. 打开文件
  int fd = fs_open(filename, 0, 0);

  // 2. 读取 ELF Header
  Elf_Ehdr ehdr;
  fs_read(fd, &ehdr, sizeof(Elf_Ehdr));
  
  // 检查魔数 (0x7f, 'E', 'L', 'F')
  assert(*(uint32_t *)ehdr.e_ident == 0x464c457f);
  
  Log("ELF Header: magic=%x, entry=%x, phnum=%d", 
      *(uint32_t *)ehdr.e_ident, (uint32_t)ehdr.e_entry, ehdr.e_phnum);

  // 3. [PA4] 创建地址空间
  // 申请一级页表，并将内核空间的映射 (恒等映射) 拷贝过去
  protect(&pcb->as);

  // [PA4] 初始化进程的堆顶指针 (max_brk)
  // 堆通常紧接着代码段/数据段之后生长
  pcb->max_brk = 0;

  // 4. 遍历 Program Headers
  Elf_Phdr phdr;
  for (int i = 0; i < ehdr.e_phnum; i++) {
    fs_lseek(fd, ehdr.e_phoff + i * ehdr.e_phentsize, SEEK_SET);
    fs_read(fd, &phdr, sizeof(Elf_Phdr));

    if (phdr.p_type == PT_LOAD) {
      // 获取该段的虚拟地址范围
      uintptr_t vaddr = phdr.p_vaddr;
      uintptr_t memsz = phdr.p_memsz;
      uintptr_t filesz = phdr.p_filesz;
      uintptr_t file_offset = phdr.p_offset;

      // 算出该段覆盖了哪些虚拟页 (Page)
      // page_start: 向下对齐到页边界
      // page_end:   向上对齐到页边界
      void *page_start = (void *)(vaddr & ~PGMASK);
      void *page_end   = (void *)((vaddr + memsz + PGSIZE - 1) & ~PGMASK);

      Log("Loading segment: [%x, %x) mapped to pages [%x, %x)", 
          vaddr, vaddr + memsz, page_start, page_end);

      // [PA4] 更新 max_brk
      // 找到所有段中最大的结束地址，以此作为堆的起始位置
      uintptr_t virt_end = vaddr + memsz;
      if (virt_end > pcb->max_brk) {
        pcb->max_brk = virt_end;
      }

      // 按页循环处理：申请物理页 -> 建立映射 -> 拷贝数据
      for (void *va = page_start; va < page_end; va += PGSIZE) {
        // A. 申请一页物理内存 (Physical Address)
        void *pa = new_page(1);

        // B. 建立映射: Virtual Address (va) -> Physical Address (pa)
        // 这样用户程序访问 va 时，实际上是访问 pa
        map(&pcb->as, va, pa, 0);

        // C. 将文件内容读入物理页
        // 注意：因为我们在内核态 (Nanos-lite)，且内核有恒等映射，
        // 所以我们可以直接写入物理地址 pa。
        
        uintptr_t page_addr = (uintptr_t)va;
        
        // 计算文件中的偏移量和长度，处理段不对齐的情况
        uintptr_t seg_start = vaddr;
        uintptr_t seg_end   = vaddr + filesz;
        uintptr_t cur_start = page_addr;
        uintptr_t cur_end   = page_addr + PGSIZE;

        // 计算当前页和文件段内容的重叠区域 [copy_start, copy_end)
        uintptr_t copy_start = (seg_start > cur_start) ? seg_start : cur_start;
        uintptr_t copy_end   = (seg_end   < cur_end)   ? seg_end   : cur_end;

        if (copy_end > copy_start) {
          // 移动文件指针到对应位置
          fs_lseek(fd, file_offset + (copy_start - vaddr), SEEK_SET);
          
          // 读取数据直接写入物理页的对应偏移处
          fs_read(fd, (void *)((uintptr_t)pa + (copy_start - cur_start)), copy_end - copy_start);
        }
        
        // BSS段 (memsz > filesz 的部分) 不需要额外处理，
        // 因为 new_page 内部已经做了 memset(0)，物理页天然就是清零的。
      }
    }
  }

  // [PA4] 堆顶对齐
  // 将 max_brk 向上对齐到页边界。
  // 这样当用户调用 sbrk 增长堆时，mm_brk 可以直接从下一页开始分配。
  pcb->max_brk = (pcb->max_brk + PGSIZE - 1) & ~PGMASK;
  Log("Heap start at max_brk = %x", pcb->max_brk);

  // 5. [PA4] 分配用户栈 (User Stack)
  // 用户栈位于用户空间的最高处 (pcb->as.area.end)
  // 因为 ELF 里没有描述栈，所以必须手动申请并映射，否则压栈时会 Page Fault
  void *stack_end = pcb->as.area.end;
  for (int i = 0; i < 8; i++) { // 分配 32KB 栈空间
    void *pa = new_page(1);
    // 栈向下生长：从 end - 4KB, end - 8KB ... 依次向下映射
    void *va = (void *)((uintptr_t)stack_end - (i + 1) * PGSIZE);
    map(&pcb->as, va, pa, 0);
  }

  fs_close(fd);

  return ehdr.e_entry;
}

void naive_uload(PCB *pcb, const char *filename) {
  panic("naive_uload should not be used in PA4");
}

void context_uload(PCB *pcb, const char *filename, char *const argv[]) {
  // 1. 加载程序
  uintptr_t entry = loader(pcb, filename);

  // 2. 构造内核栈上下文
  Area kstack;
  kstack.start = (void *)pcb->stack;
  kstack.end   = (void *)pcb->stack + STACK_SIZE;

  // 3. 生成基础上下文
  pcb->cp = ucontext(&pcb->as, kstack, (void *)entry);

  // 4. [关键修复] 初始化用户栈 (User Stack Setup)
  // ucontext 把 sp 设为了 as->area.end (比如 0x80000000)
  // 我们需要把参数填进去，并把 sp 往下移，确保 sp 指向的是用户可访问的内存

  // 这里的 sp 是"物理地址"视角吗？不是！
  // 此时我们是在内核态，虽然 satp 还没切，但我们已经通过 map 建立了映射。
  // 要往用户栈写数据，我们需要知道用户栈对应的"物理页"在哪里。
  // 但为了简化，我们可以利用一个特性：new_page 分配出的物理页，在内核里也是可以把物理地址当虚拟地址直接写的。
  
  // 这种写法比较绕，我们采用最简单粗暴的方法：
  // 假装 argc = 0，argv = NULL，envp = NULL
  // 栈布局(从高到低): [envp:NULL] [argv:NULL] [argc:0] <- sp
  
  // 获取用户栈底的物理页 (在 loader 里我们分配了8页，最后分配的那页对应最高地址)
  // 但 loader 并没有把物理地址传出来。
  
  // [替代方案] 修改 sp 寄存器
  // 让 sp 指向 0x80000000 下方一点点，并填入 0
  // 由于我们没法方便地直接写用户虚拟地址（需要切 satp），
  // 我们这里只做最小修改：把 sp 往下移，避开内核边界。
  
  pcb->cp->gpr[2] -= sizeof(uintptr_t) * 4; // sp -= 16 (预留空间)
  
  // 此时 sp 指向了 0x7FFFFFF0。
  // 这个地址对应的物理页，new_page 已经清零过了。
  // 所以 _start 读取 0(sp) 会读到 0 (即 argc=0)。
  // 这是安全的！
}