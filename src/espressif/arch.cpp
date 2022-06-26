
#include <stdint.h>

#include "panglos/arch.h"

#if defined(ARCH_ESPRESSIF)

extern "C" bool arch_in_irq()
{
    // TODO
    return false;
}

#endif  //  ARCH_ESPRESSIF

//  FIN
