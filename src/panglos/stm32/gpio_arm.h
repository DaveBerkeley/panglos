
#pragma once

#if defined(STM32F1xx)
#include "stm32f1xx_hal_gpio.h"
#endif
#if defined(STM32F4xx)
#include "stm32f4xx_hal_gpio.h"
#endif

#include "panglos/drivers/gpio.h"

namespace panglos {

//  Should probably just use the HAL's
//  GPIO_InitTypeDef*
//  and add some convenience functions?

void    gpio_enable_clock(GPIO_TypeDef *port);

void    gpio_init(GPIO_TypeDef *_port, uint16_t _pin, uint32_t mode, uint32_t alt_fn=0, uint16_t pull=0);

GPIO   *gpio_create(GPIO_TypeDef *_port, uint16_t _pin, uint32_t mode, uint32_t alt_fn=0, uint16_t pull=0);

const char *gpio_portstr(GPIO_TypeDef *port);

// mark the pin as in use
void    gpio_alloc(GPIO_TypeDef *_port, uint16_t _pin);

struct Pin {
    GPIO_TypeDef *port;
    uint16_t pin;
    uint32_t mode;
    uint32_t alt;
    uint16_t pull;
};

void gpio_init(const struct Pin *gpio);

} // namespace panglos

// FIN
