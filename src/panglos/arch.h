
    /*
     *  Architecture File
     *
     *  Allows access to target specific functions.
     *
     *  eg. is_in_irq() which could be used on an esp32 framework
     *  but could be used on different processor architectures.
     */

#if defined(ARCH_RISCV32)
#include "panglos/riscv32/arch.h"
#endif

#if defined(ARCH_XTENSA)
#include "panglos/espressif/arch.h"
#endif

//  FIN
