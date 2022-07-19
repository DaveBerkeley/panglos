
#if !defined(__PANGLOS_STM32_ARCH__)
#define __PANGLOS_STM32_ARCH__

    /*
     *
     */

#if defined(__cplusplus)
extern "C" {
#endif

bool arch_in_irq();

uint32_t arch_disable_irq();
void arch_restore_irq(uint32_t was);

#if defined(__cplusplus)
}
#endif

#endif  //  __PANGLOS_STM32_ARCH__

//  FIN
