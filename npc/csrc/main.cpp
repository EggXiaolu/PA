#include "Vtop.h"
#include "verilated.h"
#include "verilated_vcd_c.h"
#include <nvboard.h>
#include <stdio.h>
#include <stdlib.h>

int time_count = 0;

VerilatedContext *contextp = new VerilatedContext;
Vtop *top = new Vtop{contextp};
VerilatedVcdC *tfp = new VerilatedVcdC;

void nvboard_bind_all_pins(Vtop *top);

static void single_cycle() {
    int a = rand() & 1;
    int b = rand() & 1;
    top->a = a;
    top->b = b;
    top->eval();
    printf("a = %d, b = %d, f = %d\n", a, b, top->f);

    tfp->dump(contextp->time());
    contextp->timeInc(1);

    assert(top->f == (a ^ b));
}

int main(int argc, char **argv) {

    contextp->commandArgs(argc, argv);
    contextp->traceEverOn(true);
    top->trace(tfp, 0);
    tfp->open("wave.vcd");

    nvboard_bind_all_pins(top);
    nvboard_init();

    while (time_count < 100000000 && !contextp->gotFinish()) {
        single_cycle();
        time_count++;
        nvboard_update();
        // sleep(1);
    }

    nvboard_quit();
    delete top;
    tfp->close();
    delete contextp;
    return 0;
}