
#pragma once

#include <driver/rmt.h>

#include "panglos/drivers/led_strip.h"

namespace panglos {

class RmtLedStrip : public LedStrip
{
    int nleds;
    int bits_per_led;

    rmt_item32_t *data;
    rmt_item32_t on;
    rmt_item32_t off;

    rmt_channel_t chan;
    gpio_num_t gpio;

public:
    enum Type { WS2812B, SK68XX };

    RmtLedStrip(int _nleds, int _bits_per_led=24, Type type=WS2812B);
    ~RmtLedStrip();

    bool init(rmt_channel_t _chan, gpio_num_t _gpio);

    static uint32_t rgb(uint8_t r, uint8_t g, uint8_t b);

    virtual void set(int led, uint8_t r, uint8_t g, uint8_t b) override;
    virtual void set_all(uint8_t r, uint8_t g, uint8_t b) override;
    virtual bool send() override;
    virtual int num_leds() override;
};

}   //  namespace panglos

//  FIN
