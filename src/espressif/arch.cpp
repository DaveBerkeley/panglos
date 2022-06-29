
#include <stdint.h>

#include "panglos/debug.h"

#include "panglos/arch.h"

#if defined(ARCH_ESPRESSIF)

extern "C" bool arch_in_irq()
{
    // TODO
    return false;
}

extern "C" uint32_t arch_disable_irq()
{
    // TODO
    return 0;
}

extern "C" void arch_restore_irq(uint32_t s)
{
    // TODO
    IGNORE(s);
}

#endif  //  ARCH_ESPRESSIF

//  FIN
