
#if !defined(__DAC_H__)
#define __DAC_H__

#include <stdint.h>

namespace panglos {

class DMA;

#if defined(DAC)
#undef DAC
#endif

class DAC
{
public:

    enum ID {
        DAC_1,
        //DAC_2
    };

    enum Channel {
        CHAN_1,
        CHAN_2
    };

    enum Align {
        ALIGN_8,
        ALIGN_12_L,
        ALIGN_12_R,
    };

    virtual void init(Timer *timer) = 0;

    virtual void start() = 0;
    virtual void stop() = 0;
    virtual void start_dma(Semaphore *done, uint32_t* data, uint32_t Length) = 0;
    virtual void stop_dma() = 0;

    virtual void set(uint16_t value) = 0;

    virtual void link(DMA *dma) = 0;

    virtual uint32_t wait_dma() = 0;

    static DAC *create(ID id, Channel chan, Align align);
};

}   //  namespace panglos

#endif // __DAC_H__

//  FIN
