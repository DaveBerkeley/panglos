
#include <stdint.h>
#include <string.h>

#include "esp_idf_version.h"

#include "driver/gpio.h"

#if (ESP_IDF_VERSION_MAJOR == 4)
#include "driver/rmt.h"
#elif (ESP_IDF_VERSION_MAJOR == 5)
#include "hal/rmt_types.h"
#endif

#include "panglos/debug.h"

#include "panglos/esp32/hal.h"
#include "panglos/esp32/rmt_strip.h"

namespace panglos {

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
    
uint32_t RmtLedStrip::rgb(uint8_t r, uint8_t g, uint8_t b)
{
    return (uint32_t(g & 0xff) << 16) + (uint32_t(r & 0xff) << 8) + uint32_t(b & 0xff);
}

void RmtLedStrip::set_all(uint8_t r, uint8_t g, uint8_t b)
{
    for (int i = 0; i < num_leds(); i++)
    {
        set(i, r, g, b);
    }
}

    /*
     *
     */

class BaseRmt : public RmtLedStrip
{
public:
    int nleds;
    int bits_per_led;
    gpio_num_t gpio;
    
#if (ESP_IDF_VERSION_MAJOR == 4)
    typedef rmt_item32_t rmt_bit_t;
#elif (ESP_IDF_VERSION_MAJOR == 5)
    typedef rmt_symbol_word_t rmt_bit_t;
#endif
    rmt_bit_t *data;
    rmt_bit_t on;
    rmt_bit_t off;

    BaseRmt(int _nleds, int _bits_per_led, Type type)
    :   nleds(_nleds),
        bits_per_led(_bits_per_led)
    {
        data = new rmt_bit_t[nleds * bits_per_led];

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

    ~BaseRmt()
    {
        delete[] data;
    }

    virtual int num_leds() override { return  nleds; }   

    virtual void set(int led, uint8_t r, uint8_t g, uint8_t b) override
    {
        ASSERT(led >= 0);
        ASSERT(led < nleds);
        const int idx = led * bits_per_led;

        rmt_bit_t *out = & data[idx];
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
};

    /*
     *
     */

#if (ESP_IDF_VERSION_MAJOR == 4)

#include "driver/rmt.h"

    /*
     * Example adapted from :
     * https://github.com/JSchaenzle/ESP32-NeoPixel-WS2812-RMT/blob/master/README.md
     */

class _RmtLedStrip : public BaseRmt
{
    rmt_channel_t chan;
    gpio_num_t gpio;

public:
    _RmtLedStrip(int _nleds, int _bits_per_led, Type type);

    virtual bool init(uint32_t chan, uint32_t gpio) override;
    virtual bool send() override;
};

_RmtLedStrip::_RmtLedStrip(int nleds, int bits_per_led, Type type)
:   BaseRmt(nleds, bits_per_led, type)
{
}

bool _RmtLedStrip::init(uint32_t _chan, uint32_t _gpio)
{
    chan = (rmt_channel_t) _chan;
    gpio = (gpio_num_t) _gpio;
    
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

bool _RmtLedStrip::send()
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

#endif  //  ESP_IDF_VERSION_MAJOR == 4

    /*
     *
     */

#if (ESP_IDF_VERSION_MAJOR == 5)

#include "driver/rmt_tx.h"

class _RmtLedStrip : public BaseRmt
{
    rmt_channel_handle_t handle;
    rmt_encoder_handle_t encoder_handle;

public:
    _RmtLedStrip(int nleds, int bits_per_led, Type type);
    ~_RmtLedStrip();

    virtual bool init(uint32_t chan, uint32_t gpio) override;

    virtual bool send() override;
};

_RmtLedStrip::_RmtLedStrip(int nleds, int bits_per_led, Type type)
:   BaseRmt(nleds, bits_per_led, type),
    handle(0),
    encoder_handle(0)
{
}

_RmtLedStrip::~_RmtLedStrip()
{
    esp_err_t err = rmt_del_channel(handle);
    if (err != ESP_OK)
    {
        PO_ERROR("%s", lut(panglos::err_lut, (err)));
    }

    err = rmt_del_encoder(encoder_handle);
    if (err != ESP_OK)
    {
        PO_ERROR("%s", lut(panglos::err_lut, (err)));
    }
}

bool _RmtLedStrip::init(uint32_t chan, uint32_t gpio)
{
    PO_DEBUG("");

    rmt_tx_channel_config_t config = {
        .gpio_num = (gpio_num_t) gpio,
        .clk_src = RMT_CLK_SRC_DEFAULT, // select source clock
        .resolution_hz = int(CK_FREQ),
        .mem_block_symbols = 64, // increase the block size can make the LED less flickering
        .trans_queue_depth = 4, // set the number of transactions that can be pending in the background
    };
    esp_err_t err = rmt_new_tx_channel(& config, & handle);
    if (err != ESP_OK)
    {
        PO_ERROR("rmt_new_tx_channel() %s", lut(panglos::err_lut, (err)));
        return false;
    }

    err = rmt_enable(handle);
    if (err != ESP_OK)
    {
        PO_ERROR("rmt_enable() %s", lut(panglos::err_lut, (err)));
        return false;
    }

    rmt_copy_encoder_config_t copy_config;
    err = rmt_new_copy_encoder(& copy_config, & encoder_handle);
    if (err != ESP_OK)
    {
        PO_ERROR("rmt_new_copy_encoder() %s", lut(panglos::err_lut, (err)));
        return false;
    }

    return true;
}

bool _RmtLedStrip::send()
{
    esp_err_t err = rmt_tx_wait_all_done(handle, 1000);
    if (err != ESP_OK)
    {
        PO_ERROR("%s", lut(panglos::err_lut, (err)));
        return false;
    }

    rmt_transmit_config_t config = {
        .loop_count = 0, // no loop
    };

    size_t s = sizeof(rmt_bit_t) * nleds * bits_per_led;
    err = rmt_transmit(handle, encoder_handle, data, s, & config);
    if (err != ESP_OK)
    {
        PO_ERROR("rmt_transmit() %s", lut(panglos::err_lut, (err)));
        return false;
    }
    return true;
}

#endif

    /*
     *
     */

RmtLedStrip *RmtLedStrip::create(int nleds, int bits_per_led, Type type)
{
    return new _RmtLedStrip(nleds, bits_per_led, type);
}

}   //  namespace panglos

//  FIN
