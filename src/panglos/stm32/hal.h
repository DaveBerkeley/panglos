
#if !defined(__STM32_HAL_H__)
#define __STM32_HAL_H__

// The VECTACTIVE field of the ICSR (Interrupt control and state register)
// is set to show the active exception number

#if defined(STM32F1xx) || defined(STM32F4xx)

#define NVIC_ICSR   (*((volatile uint32_t *) 0xe000ed04))
#define IS_IN_IRQ() ((NVIC_ICSR & 0x1FFUL) != 0)

#endif

#endif // __STM32_HAL_H__

//  FIN
