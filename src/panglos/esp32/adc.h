    /*
     *
     */

#pragma once

#include "driver/adc.h"

#include "panglos/drivers/adc.h"

class ESP_ADC2 : public panglos::ADC
{
    adc_bits_width_t width;

public:
    ESP_ADC2();

    void set_atten(uint8_t chan, adc_atten_t atten);
    virtual uint16_t read(uint8_t chan) override;
};

//  FIN
