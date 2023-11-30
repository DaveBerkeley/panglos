
    /*
     *
     */

#include <driver/adc.h>

#include "panglos/debug.h"

#include "panglos/esp32/adc.h"

static adc2_channel_t get_chan(uint8_t chan)
{
    const adc2_channel_t chans[] = {
        ADC2_CHANNEL_0,
        ADC2_CHANNEL_1,
        ADC2_CHANNEL_2,
        ADC2_CHANNEL_3,
        ADC2_CHANNEL_4,
        ADC2_CHANNEL_5,
        ADC2_CHANNEL_6,
        ADC2_CHANNEL_7,
        ADC2_CHANNEL_8,
        ADC2_CHANNEL_9,
    };
    ASSERT(chan < (sizeof(chans) / sizeof(chans[0])));
    return chans[chan];
}


ESP_ADC2::ESP_ADC2()
:   width(ADC_WIDTH_BIT_12)
{
}

void ESP_ADC2::set_atten(uint8_t chan, adc_atten_t atten)
{
    esp_err_t r = adc2_config_channel_atten(get_chan(chan), atten);
    ASSERT(r == ESP_OK);
}

uint16_t ESP_ADC2::read(uint8_t chan)
{
    int rd;
    esp_err_t r = adc2_get_raw(get_chan(chan), width, & rd);
    ASSERT(r == ESP_OK);
    return rd;
}

//  FIN
