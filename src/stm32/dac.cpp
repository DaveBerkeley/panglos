
#include <string.h>

#include "panglos/stm32/stm32fxxx_hal.h"

#include <stm32f4xx_hal_dac.h>

#include <panglos/debug.h>

#include <panglos/gpio.h>
#include <panglos/stm32/gpio_arm.h>

#include <panglos/dac.h>
#include <panglos/dma.h>

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
        gpio_alloc(port, pin);

        // enable DAC APB clock
        __HAL_DAC_ENABLE(& handle, channel);

        // configure GPIO
        GPIO_InitTypeDef gpio = {0};

        gpio.Pin = pin;
        gpio.Speed = GPIO_SPEED_FREQ_LOW;
        gpio.Mode = GPIO_MODE_ANALOG;
        gpio.Pull = GPIO_PULLUP;

        HAL_GPIO_Init(port, & gpio);

        //  configure handle 
        HAL_StatusTypeDef okay;

        okay = HAL_DAC_Init(& handle);
        ASSERT(okay == HAL_OK);

        // configure channel
        DAC_ChannelConfTypeDef chan;

        chan.DAC_Trigger = DAC_TRIGGER_NONE;
        chan.DAC_OutputBuffer = DAC_OUTPUTBUFFER_ENABLE;        

        okay = HAL_DAC_ConfigChannel(& handle, & chan, channel);
        ASSERT(okay == HAL_OK);
    }

    virtual void start()
    {
        HAL_StatusTypeDef okay = HAL_DAC_Start(& handle, channel);
        ASSERT(okay == HAL_OK);
    }

    virtual void stop()
    {
        HAL_StatusTypeDef okay = HAL_DAC_Stop(& handle, channel);
        ASSERT(okay == HAL_OK);
    }

    virtual void start_dma(uint32_t *data, uint32_t length)
    {
        // TODO : is this the correct 'align' value?
        HAL_StatusTypeDef okay = HAL_DAC_Start_DMA(& handle, channel, data, length, align);
        ASSERT(okay == HAL_OK);
    }

    virtual void stop_dma()
    {
        HAL_StatusTypeDef okay = HAL_DAC_Stop_DMA(& handle, channel);
        ASSERT(okay == HAL_OK);
    }

    virtual void set(uint16_t value)
    {
        HAL_StatusTypeDef okay = HAL_DAC_SetValue(& handle, channel, align, value);
        ASSERT(okay == HAL_OK);
    }
 
    virtual void link(DMA *dma)
    {
        ASSERT(dma);
        DMA_HandleTypeDef *dhma = (DMA_HandleTypeDef *) dma->get_handle();
        switch (channel)
        {
            case DAC_CHANNEL_1 : __HAL_LINKDMA(& handle, DMA_Handle1, *dhma); break;
            case DAC_CHANNEL_2 : __HAL_LINKDMA(& handle, DMA_Handle2, *dhma); break;
            default : ASSERT(0);
        }
    }
};

    /*
     *
     */

xDAC *xDAC::create(ID id, Channel chan, Align _align)
{
    uint32_t align = 0;
    uint32_t channel = 0;
    GPIO_TypeDef *port = GPIOA;
    uint32_t pin = 0;

    __HAL_RCC_GPIOA_CLK_ENABLE();

    switch (_align)
    {
        case ALIGN_8    : align = DAC_ALIGN_8B_R; break;
        case ALIGN_12_L : align = DAC_ALIGN_12B_L; break;
        case ALIGN_12_R : align = DAC_ALIGN_12B_R; break;
        default : ASSERT(0);
    }

    switch (chan)
    {
        case CHAN_1 :
        {
            channel = DAC_CHANNEL_1;
            pin = GPIO_PIN_4;
            break;
        }
        case CHAN_2 :
        {
            channel = DAC_CHANNEL_2;
            pin = GPIO_PIN_5;
            break;
        }
        default : ASSERT(0);
    }

    DAC_TypeDef *instance = 0;

    switch (id)
    {
        case DAC_1 : 
        {
            __HAL_RCC_DAC_CLK_ENABLE();
            instance = DAC1; 
            break;
        }
        //case DAC_2 : instance = DAC2; break;
        default : ASSERT(0);
    }
    return new Arm_DAC(instance, channel, port, pin, align);
}

}   //  namespace panglos

//  FIN
