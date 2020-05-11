
#if !defined(__GPIO_ARM_H__)
#define __GPIO_ARM_H__

#if defined(STM32F1xx)
#include "stm32f1xx_hal_gpio.h"
#endif
#if defined(STM32F4xx)
#include "stm32f4xx_hal_gpio.h"
#endif

namespace panglos {

GPIO * gpio_create(GPIO_TypeDef *_port, uint16_t _pin, uint32_t mode);

} // namespace panglos

#endif // __GPIO_ARM_H__

// FIN
