
    /*
     *
     */

//#include "panglos/arch.h"

extern "C" {

void arch_in_irq()
{
    asm volatile("csrr a0, mstatus");
    asm volatile("srl a0, a0, 11"); // 0x1800 >> 11
    asm volatile("andi a0, a0, 0x3");
}

}

//  FIN
