#include "Vour.h"
#include "verilated.h"
#include "verilated_vcd_c.h"
#include <stdio.h>
#include <stdlib.h>

int time_count = 0;

int main(int argc, char **argv) {
    VerilatedContext *contextp = new VerilatedContext;
    contextp->commandArgs(argc, argv);
    Vour *top = new Vour{contextp};

    VerilatedVcdC *tfp = new VerilatedVcdC;
    contextp->traceEverOn(true);
    top->trace(tfp, 0);
    tfp->open("wave.vcd");
    while (time_count < 30 && !contextp->gotFinish()) {
        int a = rand() & 1;
        int b = rand() & 1;
        top->a = a;
        top->b = b;
        top->eval();
        printf("a = %d, b = %d, f = %d\n", a, b, top->f);

        tfp->dump(contextp->time());
        contextp->timeInc(1);

        assert(top->f == (a ^ b));

        time_count++;
    }
    delete top;
    tfp->close();
    delete contextp;
    return 0;
}