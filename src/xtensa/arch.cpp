
#if defined(ARCH_XTENSA)

#include <stdint.h>

extern "C" {
    #include <freertos/FreeRTOS.h>
    #include <freertos/portmacro.h>
}

//#include "xtensa/xtensa_api.h"
#include "xtensa/hal.h"

#include "panglos/debug.h"

#include "panglos/arch.h"

extern "C" bool arch_in_irq()
{
    return xPortInIsrContext();
}

extern "C" uint32_t arch_disable_irq()
{
    //return xt_int_disable_mask(0);
    return xthal_int_disable(0);
}

extern "C" uint32_t arch_restore_irq(uint32_t s)
{
    //return xt_int_enable_mask(s);
    return xthal_int_enable(s);
}

#endif  //  ARCH_XTENSA

//  FIN
