
#include <string.h>

#include "panglos/stm32/stm32fxxx_hal.h"

#include <stm32f4xx_hal_dac.h>

#include <panglos/debug.h>

#include <panglos/gpio.h>
#include <panglos/stm32/gpio_arm.h>

#include <panglos/dac.h>

namespace panglos {

class Arm_DAC : public xDAC
{
    DAC_HandleTypeDef handle;
    uint32_t channel;
    GPIO_TypeDef *port;
    uint32_t pin;
    uint32_t align;

public:
    Arm_DAC(DAC_TypeDef *instance, uint32_t chan, GPIO_TypeDef *_port, uint32_t _pin, uint32_t _align)
    :   channel(chan),
        port(_port),
        pin(_pin),
        align(_align)
    {
        memset(& handle, 0, sizeof(handle));
        handle.Instance = instance;
    }

    virtual void init()
    {
        // Mark the pin as allocated
        gpio_alloc(GPIOA, pin);

        // enable DAC APB clock
        __HAL_DAC_ENABLE(& handle, channel);

        // configure GPIO
        GPIO_InitTypeDef gpio = {0};

        gpio.Pin = pin;
        gpio.Speed = GPIO_SPEED_FREQ_LOW;
        gpio.Mode = GPIO_MODE_AF_PP;
        gpio.Pull = GPIO_PULLUP;

        HAL_GPIO_Init(port, & gpio);

        // 
        // HAL_DAC_Init()
        // HAL_DAC_ConfigChannel()
    }

    virtual void start()
    {
        //  HAL_DAC_Start() or HAL_DAC_Start_DMA()
    }

    virtual void stop()
    {
        //  HAL_DAC_Stop() or HAL_DAC_Stop_DMA()
    }

    virtual void set(uint16_t value)
    {
        HAL_StatusTypeDef okay = HAL_DAC_SetValue(& handle, channel, align, value);
        ASSERT(okay == HAL_OK);
    }
};

    /*
     *
     */

xDAC * xDAC::create(xDAC::ID id, uint32_t align)
{
    uint32_t channel = DAC_CHANNEL_1;
    uint32_t pin = GPIO_PIN_4;
    GPIO_TypeDef *port = GPIOA;
    DAC_TypeDef *instance = 0;

    switch (id)
    {
        case DAC_1 : instance = DAC1; break;
        //case DAC_2 : instance = DAC2; break;
        default : ASSERT(0);
    }
    return new Arm_DAC(instance, channel, port, pin, align);
}

}   //  namespace panglos

//  FIN
