
#include <string.h>

#include "panglos/stm32/stm32fxxx_hal.h"

#include <stm32f4xx_hal_dma.h>

#include <panglos/debug.h>

#include <panglos/dma.h>

namespace panglos {

    /*
     *
     */

class Arm_DMA : public DMA
{
protected:
    DMA_HandleTypeDef handle;

    Arm_DMA()
    {
        memset(& handle, 0, sizeof(handle));
    }

    virtual IRQn_Type get_irq() = 0;
    virtual void clock_enable() = 0;

    virtual void *get_handle()
    {
        return & handle;
    }

    virtual void init(int irq_level)
    {
        // enable dma interface ck
        clock_enable();

        // configure the dma
        HAL_StatusTypeDef status = HAL_DMA_Init(& handle);
        ASSERT(status == HAL_OK);

        // configure priority and enable NVIC
        IRQn_Type irq = get_irq();
        HAL_NVIC_SetPriority(irq, irq_level, 0);
        HAL_NVIC_EnableIRQ(irq);

        __HAL_DMA_ENABLE(& handle);
    }

    void set_p_align(XferSize align)
    {
        switch (align)
        {
            case XFER_8  :  handle.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;      break;
            case XFER_16 :  handle.Init.PeriphDataAlignment = DMA_PDATAALIGN_HALFWORD;  break;
            case XFER_32 :  handle.Init.PeriphDataAlignment = DMA_PDATAALIGN_WORD;      break;
            default :       ASSERT_ERROR(0, "Not supported %d", align);
        }
    }

    void set_m_align(XferSize align)
    {
        switch (align)
        {
            case XFER_8  :  handle.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;     break;
            case XFER_16 :  handle.Init.MemDataAlignment = DMA_MDATAALIGN_HALFWORD; break;
            case XFER_32 :  handle.Init.MemDataAlignment = DMA_MDATAALIGN_WORD;     break;
            default :       ASSERT_ERROR(0, "Not supported %d", align);
        }
    }
};

    /*
     *  DAC Interface
     */

class DMA_DAC : public Arm_DMA
{
public:
    DMA_DAC(XferSize xfer)
    {
        set_m_align(xfer);
        set_p_align(XFER_32);

        handle.Init.Direction = DMA_MEMORY_TO_PERIPH;
        handle.Init.PeriphInc = DMA_PINC_DISABLE;
        handle.Init.MemInc = DMA_MINC_ENABLE;
        //handle.Init.Mode = DMA_NORMAL; // or DMA_CIRCULAR, DMA_PFCTRL (periph flow control)
        // TODO : ???????????
        //handle.Init.Mode = DMA_CIRCULAR;
        handle.Init.Mode = DMA_PFCTRL; //  (periph flow control)
        handle.Init.Priority = DMA_PRIORITY_HIGH;

        handle.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
        handle.Init.FIFOThreshold = DMA_FIFO_THRESHOLD_HALFFULL;
        handle.Init.MemBurst = DMA_MBURST_SINGLE;
        handle.Init.PeriphBurst = DMA_PBURST_SINGLE;
    }
};

    /*
     *
     */

static uint32_t channel(DMA::Chan chan)
{
    switch (chan)
    {
        case DMA::CHAN_0 : return DMA_CHANNEL_0;
        case DMA::CHAN_1 : return DMA_CHANNEL_1;
        case DMA::CHAN_2 : return DMA_CHANNEL_2;
        case DMA::CHAN_3 : return DMA_CHANNEL_3;
        case DMA::CHAN_4 : return DMA_CHANNEL_4;
        case DMA::CHAN_5 : return DMA_CHANNEL_5;
        case DMA::CHAN_6 : return DMA_CHANNEL_6;
        case DMA::CHAN_7 : return DMA_CHANNEL_7;
    }
    ASSERT(0);
    return 0;
} 

    /*
     *
     */

#if defined(STM32F1xx)

class DMA_DAC1 : public DMA_DAC 
{
public:
    DMA_DAC1(XferSize xfer)
    :   DMA_DAC(xfer)
    {
        handle.Instance = DMA2_Channel3;
        handle.Init.Channel = DMA_CHANNEL_3;
    }

    virtual IRQn_Type get_irq()
    {
        return DMA2_Channel3_IRQn;
    }
 
    virtual void clock_enable()
    {
        __HAL_RCC_DMA2_CLK_ENABLE();
    }
};

extern "C" void DMA2_Channel3_IRQHandler(void)
{
    ASSERT(0);
    //HAL_DMA_IRQHandler(dma_dac1);
}

#endif

    /*
     *
     */

#if defined(STM32F4xx)

static DMA_HandleTypeDef *dma1_s5;

class DMA_DAC1 : public DMA_DAC
{
    // DMA1, Stream 5
    //  Channel 5 is TIM3_CH2
    //  Channel 7 is DAC1
    // See table 28 of RM0390

public:
    DMA_DAC1(XferSize xfer, Chan chan)
    :   DMA_DAC(xfer)
    {
        handle.Instance = DMA1_Stream5;
        handle.Init.Channel = channel(chan);
        dma1_s5 = & handle;
    }

    virtual ~DMA_DAC1()
    {
        dma1_s5 = 0;
    }

    virtual IRQn_Type get_irq()
    {
        return DMA1_Stream5_IRQn;
    }
 
    virtual void clock_enable()
    {
        __HAL_RCC_DMA1_CLK_ENABLE();
    }
};

extern "C" void DMA1_Stream5_IRQHandler(void)
{
    ASSERT(dma1_s5);
    HAL_DMA_IRQHandler(dma1_s5);
}

#endif

    /*
     *
     */

DMA * DMA::create_DAC1(XferSize xfer, Chan chan)
{
    return new DMA_DAC1(xfer, chan);
}

}   // namespace panglos

//  FIN
