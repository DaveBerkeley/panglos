
#if !defined(__STM32_HAL_H__)
#define __STM32_HAL_H__

#include "stm32fxxx_hal.h"

namespace panglos {

// The VECTACTIVE field of the ICSR (Interrupt control and state register)
// is set to show the active exception number

#if defined(STM32F1xx) || defined(STM32F4xx)

#define NVIC_ICSR   (*((volatile uint32_t *) 0xe000ed04))
#define IS_IN_IRQ() ((NVIC_ICSR & 0x1FFUL) != 0)

    /*
     *
     */

typedef struct {
    GPIO_TypeDef *port;
    uint32_t pin;
}   PortPin;

#define INIT_AF_GPIO(pp) \
    { \
        GPIO_InitTypeDef gpio_def; \
      \
        gpio_def.Pin   = (pp)->pin; \
        gpio_def.Mode  = GPIO_MODE_AF_PP; \
        gpio_def.Pull  = GPIO_PULLUP; \
        gpio_def.Speed = GPIO_SPEED_FREQ_HIGH; \
        HAL_GPIO_Init((pp)->port, & gpio_def); \
    }

#define INIT_AF_GPIOs(map) \
    for (const PortPin *pp = (map); pp->port; pp++) \
    { \
        INIT_AF_GPIO(pp); \
    }

#endif

}   // namespace panglos

#endif // __STM32_HAL_H__

//  FIN
