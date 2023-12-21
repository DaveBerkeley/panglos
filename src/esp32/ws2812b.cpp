
#include <stdint.h>
#include <string.h>

#include "panglos/debug.h"

#include "panglos/esp32/hal.h"
#include "panglos/esp32/ws2812b.h"

namespace panglos {

    /*
     * Example adapted from :
     * https://github.com/JSchaenzle/ESP32-NeoPixel-WS2812-RMT/blob/master/README.md
     */

#define CK_FREQ 80e6
#define T10NS (10e-9 * CK_FREQ)

#define WS2812B_T1H int(T10NS * 80) // 1 bit high time
#define WS2812B_T1L int(T10NS * 45) // 1 bit low time
#define WS2812B_T0H int(T10NS * 40) // 0 bit high time
#define WS2812B_T0L int(T10NS * 85) // 0 bit low time

#define SK68XX_T1H int(T10NS * 64) // 1 bit high time
#define SK68XX_T1L int(T10NS * 70) // 1 bit low time
#define SK68XX_T0H int(T10NS * 30) // 0 bit high time
#define SK68XX_T0L int(T10NS * 100) // 0 bit low time

    /*
     *
     */

RmtLedStrip::RmtLedStrip(int _nleds, int _bits_per_led, Type type)
:   nleds(_nleds),
    bits_per_led(_bits_per_led),
    data(0)
{
    // reset pulse followed by one rmt_item32_t per bit, per led
    const int num = nleds * bits_per_led;
    data = new rmt_item32_t[num];

    switch (type)
    {
        case SK68XX :
        {
            on.duration0 = SK68XX_T1H;
            on.level0 = 1;
            on.duration1 = SK68XX_T1L;
            on.level1 = 0;
            off.duration0 = SK68XX_T0H;
            off.level0 = 1;
            off.duration1 = SK68XX_T0L;
            off.level1 = 0;
            break;
        }
        case WS2812B :
        {
            on.duration0 = WS2812B_T1H;
            on.level0 = 1;
            on.duration1 = WS2812B_T1L;
            on.level1 = 0;
            off.duration0 = WS2812B_T0H;
            off.level0 = 1;
            off.duration1 = WS2812B_T0L;
            off.level1 = 0;
            break;
        }
        default :
            ASSERT(0);
    }
}

RmtLedStrip::~RmtLedStrip()
{
    delete[] data;
}

bool RmtLedStrip::init(rmt_channel_t _chan, gpio_num_t _gpio)
{
    chan = _chan;
    gpio = _gpio;
    
    esp_err_t err;

    rmt_config_t param = {
        .rmt_mode = RMT_MODE_TX,
        .channel = chan,
        .gpio_num = gpio,
        .clk_div = 1,
        .mem_block_num = 2, // ??
        .flags = 0, // ??
        .tx_config = {
            .idle_level = RMT_IDLE_LEVEL_LOW,
            .carrier_en = false,
            .loop_en = false,
        },
    };

    err = rmt_config(& param);
    if (err != ESP_OK)
    {
        PO_ERROR("%s", lut(panglos::err_lut, (err)));
        return false;
    }

    err = rmt_driver_install(chan, 0, 0);
    if (err != ESP_OK)
    {
        PO_ERROR("%s", lut(panglos::err_lut, (err)));
        return false;
    }

    return true;
}

uint32_t RmtLedStrip::rgb(uint8_t r, uint8_t g, uint8_t b)
{
    return (uint32_t(g & 0xff) << 16) + (uint32_t(r & 0xff) << 8) + uint32_t(b & 0xff);
}

void RmtLedStrip::set(int led, uint8_t r, uint8_t g, uint8_t b)
{
    ASSERT(led >= 0);
    ASSERT(led < nleds);
    const int idx = led * bits_per_led;

    rmt_item32_t *out = & data[idx];
    uint32_t mask = 1 << (bits_per_led - 1);

    const uint32_t state = rgb(r, g, b);

    while (mask)
    {
        const bool bit = state & mask;
        memcpy(out, bit ? & on : & off, sizeof(on));
        out += 1;
        mask >>= 1;
    }
}

void RmtLedStrip::set_all(uint8_t r, uint8_t g, uint8_t b)
{
    for (int i = 0; i < num_leds(); i++)
    {
        set(i, r, g, b);
    }
}

bool RmtLedStrip::send()
{
    esp_err_t err = rmt_wait_tx_done(chan, portMAX_DELAY);
    if (err != ESP_OK)
    {
        PO_ERROR("%s", lut(panglos::err_lut, (err)));
        return false;
    }

    err = rmt_write_items(chan, data, nleds * bits_per_led, false);
    if (err != ESP_OK)
    {
        PO_ERROR("%s", lut(panglos::err_lut, (err)));
        return false;
    }

    return true;
}

int RmtLedStrip::num_leds()
{
    return nleds;
}

}   //  namespace panglos

//  FIN
