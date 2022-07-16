

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

    enum Chan {
        CHAN_0,
        CHAN_1,
        CHAN_2,
        CHAN_3,
        CHAN_4,
        CHAN_5,
        CHAN_6,
        CHAN_7,
    };

public:
    virtual ~DMA() {}

    virtual void init(int irq_level) = 0;

    // return the DMA handle
    virtual void *get_handle() = 0;

    /*
     *  Hardware specific instances of DMA / Stream / Channel
     */

    static DMA *create_DAC1(XferSize xfer, Chan chan);
};

}   //  namespace panglos

#endif // __DMA_H__

//  FIN
