#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <nvboard.h>
#include <unistd.h>  // 引入usleep函数（Linux/macOS）

#include "Vtop.h"
#include "verilated.h"
#include "verilated_fst_c.h"

extern "C" void load_img(const char *filename);
void nvboard_bind_all_pins(Vtop* top);

int main(int argc, char** argv) {
    if (argc < 2) {
        printf("Usage: %s <image.bin> [+gui]\n", argv[0]);
        exit(1);
    }

    load_img(argv[1]);

    bool use_gui = false;
    for (int i = 2; i < argc; i++) {
        if (strcmp(argv[i], "+gui") == 0)
            use_gui = true;
    }

    VerilatedContext* contextp = new VerilatedContext;
    contextp->traceEverOn(true);
    contextp->commandArgs(argc, argv);

    Vtop* top = new Vtop{contextp};

    if (use_gui) {
        nvboard_bind_all_pins(top);
        nvboard_init();
    }

    VerilatedFstC* tf = new VerilatedFstC;
    top->trace(tf, 10, 0);
    tf->open("wave.fst");

    // 延时参数：单位为微秒（1秒=1e6微秒），可调整
    const int DELAY_US = 100000;  // 100ms/步，按需修改

    while (!contextp->gotFinish()) {
        if (use_gui) nvboard_update();

        // 时钟低电平
        contextp->timeInc(1);
        top->clk = 0; top->eval(); tf->dump(contextp->time());
        usleep(DELAY_US / 2);  // 低电平期间延时（均分总延时）

        // 时钟高电平
        contextp->timeInc(1);
        top->clk = 1; top->eval(); tf->dump(contextp->time());
        usleep(DELAY_US / 2);  // 高电平期间延时

        // 可选：打印当前PC和a0寄存器值，方便调试（需确保top有对应信号导出）
        // printf("PC: 0x%08x, a0(x10): 0x%08x\n", top->pc, top->x10);
    }

    tf->close();
    delete top;
    delete contextp;
    return 0;
}