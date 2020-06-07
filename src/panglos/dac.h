
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

    virtual void init() = 0;
    virtual void start() = 0;
    virtual void stop() = 0;
    virtual void set(uint16_t value) = 0;

//#define IS_DAC_ALIGN(ALIGN) (((ALIGN) == DAC_ALIGN_12B_R)
//                             ((ALIGN) == DAC_ALIGN_12B_L)
//                             ((ALIGN) == DAC_ALIGN_8B_R))
    static xDAC *create(ID id, uint32_t align);
};

}   //  namespace panglos

#endif // __DAC_H__

//  FIN
