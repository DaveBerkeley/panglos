
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

ESP_GPIO::ESP_GPIO(int _pin, int mode, bool initial_state, bool verbose, const char *_name)
:   pin(_pin),
    output((mode & OP) & !(mode & IP)),
    verbose(verbose),
    name(_name ? _name : "")
{
    int xmode = GPIO_MODE_DISABLE;
    switch (mode & ~(PU | PD))
    {
        case IP         : xmode = GPIO_MODE_INPUT; break;
        case OP         : xmode = GPIO_MODE_OUTPUT; break;
        case (OP+OD)    : xmode = GPIO_MODE_OUTPUT_OD; break;
        case (IP+OP+OD) : xmode = GPIO_MODE_INPUT_OUTPUT_OD; break;
        case (IP+OP)    : xmode = GPIO_MODE_INPUT_OUTPUT; break;
    }

    gpio_config_t config = {
        .pin_bit_mask = (uint64_t) 1L << pin,
        .mode = (gpio_mode_t) xmode,
        .pull_up_en   = (mode & PU) ? GPIO_PULLUP_ENABLE : GPIO_PULLUP_DISABLE,
        .pull_down_en = (mode & PD) ? GPIO_PULLDOWN_ENABLE : GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };

    set(initial_state);
    esp_err_t err = gpio_config(& config);
    esp_check(err, __LINE__);
}

ESP_GPIO::~ESP_GPIO()
{
    PO_ERROR("TODO");
    ASSERT(0);
}

void ESP_GPIO::set(bool _state)
{
    if (verbose) PO_DEBUG("%s state=%d", name, _state);
    esp_err_t err = gpio_set_level((gpio_num_t) pin, _state);
    esp_check(err, __LINE__);
    state = _state;
}

bool ESP_GPIO::get()
{
    if (output)
    {
        return state;
    }
    const bool state = gpio_get_level((gpio_num_t) pin);
    if (verbose) PO_DEBUG("%s pin=%d state=%d", name, pin, state);
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
