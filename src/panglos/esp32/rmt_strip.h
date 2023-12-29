
#pragma once

#include "panglos/drivers/led_strip.h"

namespace panglos {

class RmtLedStrip : public LedStrip
{
public:
    enum Type { WS2812B, SK68XX };

    static RmtLedStrip *create(int _nleds, int _bits_per_led=24, Type type=WS2812B);

    virtual bool init(uint32_t chan, uint32_t gpio) = 0;

    static uint32_t rgb(uint8_t r, uint8_t g, uint8_t b);
};

}   //  namespace panglos

//  FIN
