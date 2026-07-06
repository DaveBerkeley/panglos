
#if defined(STM32)

#include "panglos/debug.h"

#include "panglos/stm32/hal.h"

extern "C" {
    // is this framework dependent?
    #include "cmsis_compiler.h"
}

    /*
     *
     */

extern "C" {

    /*
     *
     */

const LUT hal_lut[] = {
    {   "HAL_OK",       HAL_OK },
    {   "HAL_ERROR",    HAL_ERROR },
    {   "HAL_BUSY",     HAL_BUSY },
    {   "HAL_TIMEOUT",  HAL_TIMEOUT },
    {   0   },
};

    /*
     *
     */

bool hal_error(HAL_StatusTypeDef err)
{
    if (err == HAL_OK) return false;

    PO_ERROR("err=%d '%s'", err, lut(hal_lut, err));

    return true;
}

    /*
     *
     */

int arch_in_irq()
{
    // zero if not in an interrupt
    return __get_IPSR() != 0;
}

uint32_t arch_disable_irq()
{
    __disable_irq();
    return 0;
}

uint32_t arch_restore_irq(uint32_t)
{
    __enable_irq();
    return 0;
}

}   //  extern "C"

#endif  //  STM32

//  FIN

