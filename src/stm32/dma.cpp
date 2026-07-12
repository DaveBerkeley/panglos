
#include "panglos/stm32/hal.h"

#include "panglos/debug.h"

#include "panglos/stm32/dma.h"

namespace panglos {

    /*
     *
     */

struct IrqDef {
    DMA_Stream_TypeDef *stream;
    IRQn_Type irq;
};

static const struct IrqDef defs[] = {
    {   DMA1_Stream0, DMA1_Stream0_IRQn },
    {   DMA1_Stream1, DMA1_Stream1_IRQn },
    {   DMA1_Stream2, DMA1_Stream2_IRQn },
    {   DMA1_Stream3, DMA1_Stream3_IRQn },
    {   DMA1_Stream4, DMA1_Stream4_IRQn },
    {   DMA1_Stream5, DMA1_Stream5_IRQn },
    {   DMA1_Stream6, DMA1_Stream6_IRQn },
    {   DMA1_Stream7, DMA1_Stream7_IRQn },
    {   DMA2_Stream0, DMA2_Stream0_IRQn },
    {   DMA2_Stream1, DMA2_Stream1_IRQn },
    {   DMA2_Stream2, DMA2_Stream2_IRQn },
    {   DMA2_Stream3, DMA2_Stream3_IRQn },
    {   DMA2_Stream4, DMA2_Stream4_IRQn },
    {   DMA2_Stream5, DMA2_Stream5_IRQn },
    {   DMA2_Stream6, DMA2_Stream6_IRQn },
    {   DMA2_Stream7, DMA2_Stream7_IRQn },
    // TODO : add more DMAs
    { 0 },
};

    /*
     *
     */

IRQn_Type Dma::get_irq_type(DMA_Stream_TypeDef *stream)
{
    for (const struct IrqDef *def = defs; def->stream; def++)
        if (stream == def->stream)
            return def->irq;

    ASSERT(0);
    return IRQn_Type(0);
}

    /*
     *  Clock Enable
     */

bool Dma::ck_enable(DMA_TypeDef *dma)
{
#if defined(DMA1)
    if (dma == DMA1)
    {
        __HAL_RCC_DMA1_CLK_ENABLE();
        return true;
    }
#endif
#if defined(DMA2)
    if (dma == DMA2)
    {
        __HAL_RCC_DMA2_CLK_ENABLE();
        return true;
    }
#endif
    ASSERT(0);
    return false;
}

}   //  namespace panglos

//  FIN
