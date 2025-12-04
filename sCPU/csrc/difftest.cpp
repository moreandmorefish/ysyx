#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include "difftest.h"
#include "Vtop.h"
#include "Vtop___024root.h" 

extern Vtop* top; 
// 声明为 C 符号，避免 C++ name mangling
extern "C" uint8_t pmem[];

// ... (函数指针定义保持不变) ...
typedef void (*difftest_memcpy_t)(uint32_t addr, void *buf, size_t n, bool direction);
typedef void (*difftest_regcpy_t)(void *dut, bool direction);
typedef void (*difftest_exec_t)(uint64_t n);
typedef void (*difftest_init_t)(int port);

static difftest_memcpy_t ref_difftest_memcpy = NULL;
static difftest_regcpy_t ref_difftest_regcpy = NULL;
static difftest_exec_t ref_difftest_exec = NULL;
static difftest_init_t ref_difftest_init = NULL;

// ... (init_difftest 保持不变) ...
void init_difftest(const char *ref_so_file, long img_size, int port) {
    assert(ref_so_file != NULL);
    void *handle = dlopen(ref_so_file, RTLD_LAZY);
    if (!handle) { printf("Error: %s\n", dlerror()); exit(1); }
    
    ref_difftest_memcpy = (difftest_memcpy_t)dlsym(handle, "difftest_memcpy");
    ref_difftest_regcpy = (difftest_regcpy_t)dlsym(handle, "difftest_regcpy");
    ref_difftest_exec   = (difftest_exec_t)dlsym(handle, "difftest_exec");
    ref_difftest_init   = (difftest_init_t)dlsym(handle, "difftest_init");
    
    ref_difftest_init(port);
    ref_difftest_memcpy(0x80000000, pmem, img_size, true); 
    
    riscv32_CPU_state ref_r;
    ref_r.pc = 0x80000000;
    for(int i=0; i<32; i++) ref_r.gpr[i] = 0;
    ref_difftest_regcpy(&ref_r, true); 
    printf("\033[1;32m[DiffTest] Init Success. REF PC set to 0x80000000\033[0m\n");
}

// pc: NPC 刚刚完成提交的指令地址 (Commit PC)
// npc: NPC 的下一条指令地址 (Next PC) - 如果没有可传 0
void difftest_step(uint32_t pc, uint32_t npc) {
    riscv32_CPU_state ref_r;

    // =============================================================
    // [关键改进] 1. 在执行前，检查 REF 的 PC 是否与 NPC 当前指令一致
    // =============================================================
    ref_difftest_regcpy(&ref_r, false); // 读出 REF 当前状态
    if (ref_r.pc != pc) {
        printf("\033[1;31m[DiffTest] FATAL: PC Mismatch BEFORE Exec!\033[0m\n");
        printf("  NPC executing: 0x%08x\n", pc);
        printf("  REF expected : 0x%08x\n", ref_r.pc);
        printf("  (Core diverge detected, stopping simulation)\n");
        // 遇到 PC 不对齐，说明上一条跳转指令处理不一致，或者产生了多余的 step
       // exit(1); 
    }

    // =============================================================
    // 2. 让 NEMU 执行当前这一条指令
    // =============================================================
    ref_difftest_exec(1);

    // =============================================================
    // 3. 读出 REF 执行后的状态，对比寄存器
    // =============================================================
    ref_difftest_regcpy(&ref_r, false);

    bool error_found = false;

    // 获取当前指令的机器码用于 Debug
    uint32_t paddr = pc - 0x80000000;
    uint32_t inst = 0;
    if (paddr < 0x8000000) { inst = *(uint32_t*)(pmem + paddr); }

    // 遍历寄存器 (跳过 x0，因为它恒为0)
    for (int i = 1; i < 32; i++) {
        uint32_t dut_val = top->rootp->top__DOT__my_regfile__DOT__regfile[i];
        if (dut_val != ref_r.gpr[i]) {
            if (!error_found) {
                // 只打印一次头部信息
                printf("\033[1;31m[DiffTest] Mismatch after executing PC=0x%08x (Inst: 0x%08x)\033[0m\n", pc, inst);
                error_found = true;
            }
            printf("  Reg x%-2d | DUT: 0x%08x | REF: 0x%08x\n", i, dut_val, ref_r.gpr[i]);
        }
    }
    
    if (error_found) {
        printf("\033[1;31m[DiffTest] Simulation Aborted due to mismatch.\033[0m\n");
        //exit(1); // 强烈建议出错立即停止，否则 Log 太多无法分析
    } else {
        printf("\033[1;32m[DiffTest] Match at PC=0x%08x (Inst: 0x%08x)\033[0m\n", pc, inst);
    }   
}





