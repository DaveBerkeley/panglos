
#pragma once

#include "panglos/drivers/7-segment.h"

namespace panglos {

class SpiDevice;

class MAX7219 : public Display
{
    SpiDevice *spi;

    enum Reg {
        NOOP = 0,
        DIGIT0 = 1,
        DIGIT7 = 8,
        DECODE_MODE = 9,
        INTENSITY = 0xa,
        SCAN_LIMIT = 0xb,
        SHUTDOWN = 0xc,
        DISPLAY_TEST = 0xf,
    };

    bool write(enum Reg reg, uint8_t value);

public:

    MAX7219(SpiDevice *_spi);

    virtual void write(const char *text) override;
    virtual void set_brightness(uint8_t level) override;
    
    bool init();
};

}   //  namespace panglos

//  FIN
