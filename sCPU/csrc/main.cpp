#include<stdio.h>
#include<stdlib.h>
#include<assert.h>
#include<nvboard.h>

#include"Vtop.h"
#include"verilated.h"
#include"verilated_fst_c.h"

void nvboard_bind_all_pins(Vtop* top);


int main(int argc, char** argv) {
    VerilatedContext* contextp = new VerilatedContext;
    contextp->traceEverOn(true);
    contextp->commandArgs(argc, argv);

    Vtop* top = new Vtop{contextp};

    nvboard_bind_all_pins(top);
    nvboard_init();
    VerilatedFstC* m_trace = new VerilatedFstC;
    top->trace(m_trace, 10, 0);
    m_trace->open("wave.fst");
    
    //reset(10);
    while (!contextp->gotFinish()) {
    	//single_cycle();
    nvboard_update();

          contextp->timeInc(1);
          top->clk = 0; top->eval(); // update output
          m_trace->dump(contextp->time());
          contextp->timeInc(1);
          top->clk = 1; top->eval();
          m_trace->dump(contextp->time());
      }
      m_trace->close();
      delete top;
      delete contextp;
      return 0;
}
