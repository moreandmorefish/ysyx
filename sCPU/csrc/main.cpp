#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <nvboard.h>
#include <unistd.h>  // 引入usleep函数

#include "Vtop.h"
#include "Vtop___024root.h" // ⚠️ 新增：必须引入，否则无法访问 rootp 内部信号
#include "verilated.h"
#include "verilated_fst_c.h"
#include "difftest.h"       // ⚠️ 新增：引入 DiffTest 接口

extern "C" void load_img(const char *filename);
void nvboard_bind_all_pins(Vtop* top);

// ⚠️ 新增：将 top 指针移到全局，供 difftest.cpp 访问
Vtop* top = NULL;

// 假设你的 ram.c / ram_dpi.c 中定义了 pmem 数组，这里声明一下以便传递给 DiffTest
// 如果编译报错说找不到 pmem，请确认你的 ram 实现文件是否参与了编译
extern "C" uint8_t pmem[];

int main(int argc, char** argv) {
    if (argc < 2) {
        printf("Usage: %s <image.bin> [+gui]\n", argv[0]);
        exit(1);
    }

    // 加载镜像到内存
    load_img(argv[1]);

    bool use_gui = false;
    for (int i = 2; i < argc; i++) {
        if (strcmp(argv[i], "+gui") == 0)
            use_gui = true;
    }

    VerilatedContext* contextp = new VerilatedContext;
    contextp->traceEverOn(true);
    contextp->commandArgs(argc, argv);

    // 实例化 top (注意：top 已在全局定义)
    top = new Vtop{contextp};

    if (use_gui) {
        nvboard_bind_all_pins(top);
        nvboard_init();
    }

    // ⚠️ 新增：初始化 DiffTest
    // 1. NEMU 动态库路径 (根据你的实际路径确认)
    const char* ref_so_file = "/home/water123/pa/ysyx-workbench/nemu/build/riscv32-nemu-interpreter-so";
    // 2. 内存大小 (假设为 128MB，或者根据你的 RTL 参数设定)
    long img_size = 0x8000000; 
    // 3. 端口 (NEMU 不需要端口，传 0 即可)
    init_difftest(ref_so_file, img_size, 0);


    VerilatedFstC* tf = new VerilatedFstC;
    top->trace(tf, 10, 0);
    tf->open("wave.fst");

    // 延时参数
    const int DELAY_US = 100000; 

    // 复位逻辑 (可选：建议先复位一下)
    top->clk = 0; top->reset = 1; top->eval();
    top->clk = 1; top->reset = 1; top->eval();
    top->reset = 0;

    while (!contextp->gotFinish()) {
        if (use_gui) nvboard_update();

        // 时钟低电平
        contextp->timeInc(1);
        top->clk = 0; top->eval(); tf->dump(contextp->time());
        // 如果开启 DiffTest，建议注释掉延时，否则跑得太慢；调试时可保留
        if (use_gui) usleep(DELAY_US / 2); 

        // 时钟高电平 (上升沿，寄存器更新)
        contextp->timeInc(1);
        top->clk = 1; top->eval(); tf->dump(contextp->time());
        if (use_gui) usleep(DELAY_US / 2); 

        // ⚠️ 新增：DiffTest 钩子
        // 访问 rootp 需要 Vtop___024root.h
        if (top->rootp->top__DOT__instr_flag == 1) {
            // 获取提交的 PC (WriteBack PC)
            uint32_t commit_pc = top->rootp->top__DOT__WB_pc;
            
            // 下一条 PC (Next PC)
            // 由于 sCPU 可能没有直接输出 next_pc，且 NEMU 执行后 PC 会自动+4或跳转
            // 这里我们暂时传入 commit_pc，在 difftest_step 内部处理对比逻辑
            difftest_step(commit_pc, 0); 
        }

        // 可选：打印 (DiffTest 开启后，如果出错会自动打印，这里可以注释掉以减少刷屏)
        // printf("PC: 0x%08x\n", top->rootp->top__DOT__pc_current);
    }

    tf->close();
    delete top;
    delete contextp;
    return 0;
}