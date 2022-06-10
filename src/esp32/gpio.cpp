
    /*
     *
     */

// espidf framework
#include <driver/gpio.h>

//Panglos
#include "panglos/debug.h"

#include "panglos/esp32/hal.h"
#include "panglos/esp32/gpio.h"

    /*
     *
     */

namespace panglos {

ESP_GPIO::ESP_GPIO(gpio_num_t _pin, bool input, bool pullup, bool pulldown, bool initial_state, bool verbose)
:   pin(_pin),
    output(!input),
    verbose(verbose)
{
    gpio_config_t config = {
        .pin_bit_mask = (uint64_t) 1L << pin,
        .mode = input ? GPIO_MODE_INPUT : GPIO_MODE_OUTPUT,
        .pull_up_en = pullup ? GPIO_PULLUP_ENABLE : GPIO_PULLUP_DISABLE,
        .pull_down_en = pulldown ? GPIO_PULLDOWN_ENABLE : GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };

    esp_err_t err = gpio_config(& config);
    esp_check(err, __LINE__);
    set(initial_state);
}

ESP_GPIO::~ESP_GPIO()
{
    PO_ERROR("TODO");
    ASSERT(0);
}

void ESP_GPIO::set(bool _state)
{
    if (verbose) PO_DEBUG("state=%d", _state);
    esp_err_t err = gpio_set_level(pin, _state);
    esp_check(err, __LINE__);
    state = _state;
}

bool ESP_GPIO::get()
{
    if (output)
    {
        return state;
    }
    const bool state = gpio_get_level(pin);
    if (verbose) PO_DEBUG("pin=%d state=%d", pin, state);
    return state;
}

void ESP_GPIO::toggle()
{
    set(!get());
}

void ESP_GPIO::set_interrupt_handler(void (*fn)(void *arg), void *arg)
{
    PO_ERROR("TODO");
    ASSERT(0);
}

void ESP_GPIO::on_interrupt()
{
    PO_ERROR("TODO");
    ASSERT(0);
}

}   //  namepace panglos

//  FIN
