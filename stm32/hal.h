
#if !defined(__STM32_HAL_H__)
#define __STM32_HAL_H__

// NOTE : CORTEX M4
#define portNVIC_INT_CTRL_REG       ( * ( ( volatile uint32_t * ) 0xe000ed04 ) )

#define IS_IN_IRQ() ((portNVIC_INT_CTRL_REG & 0xFFUL) != 0)

#endif // __STM32_HAL_H__

//  FIN
