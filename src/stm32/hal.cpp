
#if defined(STM32)

extern "C" {
    // is this framework dependent?
    #include "cmsis_compiler.h"
}

    /*
     *
     */

extern "C" {

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

