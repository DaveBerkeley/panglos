
#include "panglos/stm32/stm32fxxx_hal.h"

#include <stm32f4xx_hal_dma.h>

#include <panglos/debug.h>

#include <panglos/dma.h>

namespace panglos {

    /*
     *
     */

void DMA::init(int irq_level)
{
    // enable dma interface ck
    clock_enable();

    // configure the dma
    HAL_StatusTypeDef status = HAL_DMA_Init(& handle);
    ASSERT(status == HAL_OK);

    // associate dma handle with the device handle
    link();

    // configure priority and enable NVIC
    IRQn_Type irq = get_irq();
    HAL_NVIC_SetPriority(irq, irq_level, 0);
    HAL_NVIC_EnableIRQ(irq);
}

void DMA::set_p_align(XferSize align)
{
    switch (align)
    {
        case XFER_8  :  
        {
            handle.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
            break;
        }
        case XFER_16 : 
        {
            handle.Init.PeriphDataAlignment = DMA_PDATAALIGN_HALFWORD;
            break;
        }
        case XFER_32 :
        {
            handle.Init.PeriphDataAlignment = DMA_PDATAALIGN_WORD;
            break;
        }
        default :
        {
            ASSERT_ERROR(0, "Not supported %d", align);
        }
    }
}

void DMA::set_m_align(XferSize align)
{
    switch (align)
    {
        case XFER_8  :  
        {
            handle.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
            break;
        }
        case XFER_16 : 
        {
            handle.Init.MemDataAlignment = DMA_MDATAALIGN_HALFWORD;
            break;
        }
        case XFER_32 :
        {
            handle.Init.MemDataAlignment = DMA_MDATAALIGN_WORD;
            break;
        }
        default :
        {
            ASSERT_ERROR(0, "Not supported %d", align);
        }
    }
}

    /*
     *  DAC Interface
     */

class DMA_DAC : public DMA
{
public:
    DAC_HandleTypeDef *dac;

    DMA_DAC(DAC_HandleTypeDef *_dac, XferSize xfer)
    :   dac(_dac)
    {
        set_m_align(xfer);
        set_p_align(XFER_16);

        handle.Init.Direction = DMA_MEMORY_TO_PERIPH;
        handle.Init.PeriphInc = DMA_PINC_DISABLE;
        handle.Init.MemInc = DMA_MINC_ENABLE;
        handle.Init.Mode = DMA_NORMAL; // or DMA_CIRCULAR
        handle.Init.Priority = DMA_PRIORITY_HIGH;
    }

    virtual void link()
    {
        __HAL_LINKDMA(dac, DMA_Handle1, handle);
    }
};

    /*
     *
     */

#if defined(STM32F1xx)

class DMA_DAC1 : public DMA_DAC 
{
public:
    DMA_DAC1(DAC_HandleTypeDef *_dac, XferSize xfer)
    :   DMA_DAC(_dac, xfer)
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

static DMA_DAC1 *dma_dac1 = 0;

void HAL_DAC_ConvCpltCallbackCh1(DAC_HandleTypeDef *hdac)
{
    // TODO
    ASSERT(0);
    ASSERT(hdac == & dma_dac1->handle);
}

extern "C" void DMA2_Channel3_IRQHandler(void)
{
    HAL_DMA_IRQHandler(dma_dac1);
}

#endif

    /*
     *
     */

#if defined(STM32F4xx)

class DMA_DAC1 : public DMA_DAC
{
public:
    DMA_DAC1(DAC_HandleTypeDef *_dac, XferSize xfer)
    :   DMA_DAC(_dac, xfer)
    {
        handle.Instance = DMA1_Stream5;
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

//  TODO : interrupt handler

#endif

DMA * create_DMA_DAC1(DAC_HandleTypeDef *_dac, DMA::XferSize xfer)
{
    return new DMA_DAC1(_dac, xfer);
}

}   // namespace panglos

//  FIN
