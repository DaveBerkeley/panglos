
#include <stdint.h>

#if defined(ESP32)

// espidf framework
#include <driver/gpio.h>
#include <driver/ledc.h>

#include "panglos/debug.h"
#include "panglos/time.h"
#include "panglos/esp32/hal.h"
#include "panglos/esp32/gpio.h"

#include "panglos/drivers/pwm.h"

namespace panglos {

#define esp_check(err) ASSERT_ERROR((err) == ESP_OK, "err=%s", lut(panglos::err_lut, (err)));

    /*
     *
     */

static const ledc_channel_t chans[] = 
{
    LEDC_CHANNEL_0,
    LEDC_CHANNEL_1,
    LEDC_CHANNEL_2,
    LEDC_CHANNEL_3,
    LEDC_CHANNEL_4,
    LEDC_CHANNEL_5,
};

    /*
     *
     */

PWM_Timer::PWM_Timer(int _timer, int freq, int *pins, bool _verbose)
:   timer(_timer),
    npins(0),
    verbose(_verbose)
{
    esp_err_t err;

    ledc_timer_config_t timer_config = {
        .speed_mode       = LEDC_LOW_SPEED_MODE,
        .duty_resolution  = LEDC_TIMER_13_BIT,
        .timer_num        = (ledc_timer_t) timer,
        .freq_hz          = (uint32_t) freq,
        .clk_cfg          = LEDC_AUTO_CLK
    };

    err = ledc_timer_config(& timer_config);
    esp_check(err);

    ledc_channel_config_t config = {
        .gpio_num = 0, // pin
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = (ledc_channel_t) 0, // LEDC_CHANNEL_0,
        .intr_type = LEDC_INTR_DISABLE,
        .timer_sel = (ledc_timer_t) timer,
        .duty = 0,
        .hpoint = 0,
    };

    for (int idx = 0; idx < 6 ; idx++)
    {
        if (pins[idx] == -1)
        {
            break;
        }
        if (verbose) PO_INFO("pin=%d", pins[idx]);

        ESP_GPIO::mark_used(pins[idx]);

        config.gpio_num = pins[idx];
        config.channel = chans[idx];
        err = ledc_channel_config(& config);
        esp_check(err);
        npins = idx + 1;
    }
}

bool PWM_Timer::set(int idx, uint32_t value, bool flush)
{
    IGNORE(flush);

    if ((idx < 0) || (idx >= npins))
        return false;

    if (verbose) PO_DEBUG("this=%p idx=%d value=%#x", this, idx, (int) value);
    esp_err_t err;

    err = ledc_set_duty(LEDC_LOW_SPEED_MODE, chans[idx], value);
    esp_check(err);
    err = ledc_update_duty(LEDC_LOW_SPEED_MODE, chans[idx]);
    esp_check(err);
    return true;
}

}   //  namespace panglos

#endif  //  ESP32

//  FIN
