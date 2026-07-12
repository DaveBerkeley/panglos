
#pragma once

class Dma
{
public:
    struct Config {
        DMA_TypeDef *dma;
        DMA_Stream_TypeDef *stream;
        uint32_t channel;
    };

    static IRQn_Type get_irq_type(DMA_Stream_TypeDef *stream);
    static bool ck_enable(DMA_TypeDef *dma);
};

//  FIN
