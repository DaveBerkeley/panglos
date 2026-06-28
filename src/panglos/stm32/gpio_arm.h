
#pragma once

#if defined(STM32F1xx)
#include "stm32f1xx_hal_gpio.h"
#endif
#if defined(STM32F4xx)
#include "stm32f4xx_hal_gpio.h"
#endif

#include "panglos/drivers/gpio.h"

namespace panglos {

void    gpio_enable_clock(GPIO_TypeDef *port);
void    gpio_init(GPIO_TypeDef *_port, uint16_t _pin, uint32_t mode, uint32_t alt_fn=0);
GPIO   *gpio_create(GPIO_TypeDef *_port, uint16_t _pin, uint32_t mode, uint32_t alt_fn=0);
void    gpio_alloc(GPIO_TypeDef *_port, uint16_t _pin);

} // namespace panglos

// FIN
