
#include <string.h>

#include "panglos/stm32/stm32fxxx_hal.h"

#include <stm32f4xx_hal_dac.h>

#include <panglos/debug.h>

#include <panglos/gpio.h>
#include <panglos/stm32/gpio_arm.h>

#include <panglos/dac.h>
#include <panglos/dma.h>

namespace panglos {

class Arm_DAC : public DAC
{
public:

    DAC_HandleTypeDef handle;
    uint32_t channel;
    GPIO_TypeDef *port;
    uint32_t pin;
    uint32_t align;
    Semaphore *done;

public:
    Arm_DAC(DAC_TypeDef *instance, uint32_t chan, GPIO_TypeDef *_port, uint32_t _pin, uint32_t _align)
    :   channel(chan),
        port(_port),
        pin(_pin),
        align(_align),
        done(0)
    {
        memset(& handle, 0, sizeof(handle));
        handle.Instance = instance;
    }

    virtual void init(Timer *timer)
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
        gpio.Pull = GPIO_NOPULL;

        HAL_GPIO_Init(port, & gpio);

        //  configure handle 
        HAL_StatusTypeDef okay;

        okay = HAL_DAC_Init(& handle);
        ASSERT(okay == HAL_OK);

        // configure channel
        DAC_ChannelConfTypeDef chan;

        // select trigger source
        uint32_t trigger = DAC_TRIGGER_NONE;

        // TODO : ???????
        //trigger = DAC_TRIGGER_SOFTWARE;

        if (timer)
        {
            switch (timer->get_id())
            {
                case Timer::TIMER_2 : trigger = DAC_TRIGGER_T2_TRGO; break;
                case Timer::TIMER_4 : trigger = DAC_TRIGGER_T4_TRGO; break;
                case Timer::TIMER_5 : trigger = DAC_TRIGGER_T5_TRGO; break;
                case Timer::TIMER_6 : trigger = DAC_TRIGGER_T6_TRGO; break;
                case Timer::TIMER_7 : trigger = DAC_TRIGGER_T7_TRGO; break;
                case Timer::TIMER_8 : trigger = DAC_TRIGGER_T8_TRGO; break;
                default : ASSERT_ERROR(0, "timer %d not supported", timer->get_id()+1);
            }
        }

        chan.DAC_Trigger = trigger;
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

    virtual void start_dma(Semaphore *_done, uint32_t *data, uint32_t length)
    {
        // TODO : is this the correct 'align' value?
        done = _done;
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

    void converted()
    {
        if (done)
        {
            done->post();
        }
    }

    virtual uint32_t wait_dma()
    {
        ASSERT(done);
        done->wait();
        done = 0;
        // Convert HAL error codes into our own?
        return handle.ErrorCode;
    }
};

    /*
     *
     */

static Arm_DAC *dac1 = 0;

DAC *DAC::create(ID id, Channel chan, Align _align)
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

    Arm_DAC *dac = new Arm_DAC(instance, channel, port, pin, align);

    switch (id)
    {
        case DAC_1 : dac1 = dac; break;
        default : ASSERT(0);
    }

    return dac;
}

    /*
     *  HAL Interrupt handlers
     */

static void on_irq(DAC_HandleTypeDef *hdac)
{
    if (hdac == & dac1->handle)
    {
        dac1->converted();
        return;
    }

    // TODO 
    ASSERT(0);
}

extern "C" void HAL_DAC_ErrorCallbackCh1(DAC_HandleTypeDef *hdac)
{
    on_irq(hdac);
}

extern "C" void HAL_DAC_ErrorCallbackCh2(DAC_HandleTypeDef *hdac)
{
    on_irq(hdac);
}

extern "C" void HAL_DAC_ConvCpltCallbackCh1(DAC_HandleTypeDef* hdac)
{
    on_irq(hdac);
}

extern "C" void HAL_DAC_ConvCpltCallbackCh2(DAC_HandleTypeDef* hdac)
{
    on_irq(hdac);
}

//extern "C" void HAL_DAC_ConvHalfCpltCallbackCh1(DAC_HandleTypeDef* hdac)
//{
//    ASSERT(0);
//}

//extern "C" void HAL_DAC_ConvHalfCpltCallbackCh2(DAC_HandleTypeDef* hdac)
//{
//    ASSERT(0);
//}

extern "C" void HAL_DAC_DMAUnderrunCallbackCh1(DAC_HandleTypeDef *hdac)
{
    on_irq(hdac);
}

extern "C" void HAL_DAC_DMAUnderrunCallbackCh2(DAC_HandleTypeDef *hdac)
{
    on_irq(hdac);
}

}   //  namespace panglos

//  FIN
