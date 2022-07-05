
    /*
     *
     */

#include <stdint.h>

#include "panglos/debug.h"

//#include "panglos/arch.h"

#if defined(ARCH_RISCV32)

extern "C" {

void arch_in_irq()
{
    asm volatile("csrr a0, mstatus");
    asm volatile("srl a0, a0, 11"); // 0x1800 >> 11
    asm volatile("andi a0, a0, 0x3");
}

    /*
     *  clear/set the MIE bit in mstatus
     */

uint32_t arch_disable_irq()
{
    asm volatile("csrrci a0, mstatus, 0x8");
    asm volatile("ret");
    return 0;
}

void arch_restore_irq(uint32_t s)
{
    IGNORE(s);
    asm volatile("andi a0, a0, 0x8");
    asm volatile("beq a0, zero, _leave");
    asm volatile("csrsi mstatus, 0x8");
    asm volatile("_leave:");
}

}

#endif  //  ARCH_RISCV32

//  FIN
