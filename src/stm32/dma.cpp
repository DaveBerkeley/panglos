
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
        handle.Init.Mode = DMA_NORMAL; // or DMA_CIRCULAR, DMA_PFCTRL (periph flow control)
        handle.Init.Priority = DMA_PRIORITY_HIGH;
    }
};

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
public:
    DMA_DAC1(XferSize xfer)
    :   DMA_DAC(xfer)
    {
        handle.Instance = DMA1_Stream5;
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

//  TODO : interrupt handler

#endif

    /*
     *
     */

DMA * DMA::create_DAC1(XferSize xfer)
{
    return new DMA_DAC1(xfer);
}

}   // namespace panglos

//  FIN
