
    /*
     *
     */

//#include "panglos/arch.h"

#if defined(ARCH_RISCV32)

extern "C" {

void arch_in_irq()
{
    asm volatile("csrr a0, mstatus");
    asm volatile("srl a0, a0, 11"); // 0x1800 >> 11
    asm volatile("andi a0, a0, 0x3");
}

}

#endif  //  ARCH_RISCV32

//  FIN
