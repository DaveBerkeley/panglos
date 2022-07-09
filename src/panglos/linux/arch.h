
    /*
     *
     */

#if defined(__cplusplus)
extern "C" {
#endif

bool arch_in_irq();

uint32_t arch_disable_irq();
void arch_restore_irq(uint32_t was);

// For debugging in_irq conditions
bool arch_set_in_irq(bool state);

#if defined(__cplusplus)
}
#endif

//  FIN
