
#pragma once

namespace panglos {

class LedStrip
{
public:
    virtual ~LedStrip() {}

    virtual void set(int led, uint8_t r, uint8_t g, uint8_t b) = 0;
    virtual bool send() = 0;
    virtual int num_leds() = 0;
};

}   //  namespace panglos

//  FIN
