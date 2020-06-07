
#if !defined(__DAC_H__)
#define __DAC_H__

#include <stdint.h>

namespace panglos {

class xDAC
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

    virtual void init() = 0;
    virtual void start() = 0;
    virtual void stop() = 0;
    virtual void set(uint16_t value) = 0;

    static xDAC *create(ID id, Channel chan, Align align);
};

}   //  namespace panglos

#endif // __DAC_H__

//  FIN
