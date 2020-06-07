

#if !defined(__DMA_H__)
#define __DMA_H__

namespace panglos {

class DMA
{
public:
    enum XferSize {
        XFER_8,
        XFER_16,
        XFER_32,
    };

protected:
    void set_p_align(XferSize align);
    void set_m_align(XferSize align);

public:
    DMA_HandleTypeDef handle;

    DMA();

    virtual ~DMA() {}

    virtual IRQn_Type get_irq() = 0;
    virtual void clock_enable() = 0;
    virtual void link() = 0;

    void init(int irq_level);
};

#if defined(__STM32F4xx_HAL_DAC_H)
DMA * create_DMA_DAC1(DAC_HandleTypeDef *_dac, DMA::XferSize xfer);
#endif

}   //  namespace panglos

#endif // __DMA_H__

//  FIN
