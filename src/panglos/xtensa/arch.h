
    /*
     *
     */

#if defined(__cplusplus)
extern "C" {
#endif

bool arch_in_irq();

uint32_t arch_disable_irq();
uint32_t arch_restore_irq(uint32_t was);

#if defined(__cplusplus)
}
#endif

//  FIN
