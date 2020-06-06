
#include "panglos/stm32/stm32fxxx_hal.h"

#include <stm32f4xx_hal_dma.h>

#include <panglos/debug.h>

namespace panglos {

    /*
     *
     */

class DMA
{
public:
#if defined(STM32F4xx)
    typedef DMA_Stream_TypeDef Instance;
#endif
#if defined(STM32F1xx)
    typedef DMA_Channel_TypeDef Instance;
#endif

    DMA_HandleTypeDef handle;

    DMA();

    virtual ~DMA() {}

    virtual IRQn_Type get_irq() = 0;
    virtual void clock_enable() = 0;
    virtual void link() = 0;

    void init(int irq_level)
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
};

    /*
     *
     */

class DMA_DAC : public DMA
{
public:
    DAC_HandleTypeDef *dac;

    DMA_DAC(DAC_HandleTypeDef *_dac)
    :   dac(_dac)
    {
        handle.Init.Direction = DMA_MEMORY_TO_PERIPH;
        handle.Init.PeriphInc = DMA_PINC_DISABLE;
        handle.Init.MemInc = DMA_MINC_ENABLE;
        handle.Init.PeriphDataAlignment = DMA_PDATAALIGN_HALFWORD; // ???
        handle.Init.MemDataAlignment = DMA_MDATAALIGN_HALFWORD; // ???
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
    DMA_DAC1(DAC_HandleTypeDef *_dac)
    :   DMA_DAC(_dac)
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
#endif

    /*
     *
     */

#if defined(STM32F4xx)
class DMA_DAC1 : public DMA_DAC
{
public:
    DMA_DAC1(DAC_HandleTypeDef *_dac)
    :   DMA_DAC(_dac)
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
#endif

}   // namespace panglos

//  FIN
