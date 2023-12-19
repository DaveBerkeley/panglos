
#pragma once

#include <driver/rmt.h>

#include "panglos/drivers/ws2812b.h"

namespace panglos {

class WS2812B : public LedStrip
{
    int nleds;
    int bits_per_led;

    rmt_item32_t *data;
    rmt_item32_t on;
    rmt_item32_t off;

    rmt_channel_t chan;
    gpio_num_t gpio;

public:
    WS2812B(int _nleds, int _bits_per_led=24);
    ~WS2812B();

    bool init(rmt_channel_t _chan, gpio_num_t _gpio);

    static uint32_t rgb(uint8_t r, uint8_t g, uint8_t b);

    virtual void set(int led, uint8_t r, uint8_t g, uint8_t b) override;
    virtual bool send() override;
    virtual int num_leds() override;
};

}   //  namespace panglos

//  FIN
