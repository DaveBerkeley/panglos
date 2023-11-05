
    /*
     *
     */

#if defined(ESP32)

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

static uint64_t used;

static uint64_t pin_mask(int pin)
{
    ASSERT(pin >= 0);

    return 1LL << pin;
}

void ESP_GPIO::mark_used(int pin)
{
    const uint64_t mask = pin_mask(pin);
    if (mask & used)
    {
        PO_ERROR("pin=%d used=%x", pin, (int16_t) used);
        ASSERT(0);
    }
    used |= mask;
}

void ESP_GPIO::mark_unused(int pin)
{
    const uint64_t mask = pin_mask(pin);
    used &= ~mask;
}

    /*
     *
     */

ESP_GPIO::ESP_GPIO(int _pin, int mode, bool initial_state, bool verbose, const char *_name)
:   pin(_pin),
    output((mode & OP) && !(mode & IP)),
    verbose(verbose),
    name(_name ? _name : ""),
    irq_handler(0),
    irq_arg(0)
{
    mark_used(pin);

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
    mark_unused(pin);
    //ASSERT(0);
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

static void gpio_irq_handler(void *arg)
{
    ASSERT(arg);
    GPIO *gpio = (GPIO *) arg;
    gpio->on_interrupt();
}

static bool has_irqs = false;

void ESP_GPIO::set_interrupt_handler(enum Interrupt irq, void (*fn)(void *arg), void *arg)
{
    PO_DEBUG("irq=%d fn=%p arg=%p", irq, fn, arg);
    esp_err_t err;

    irq_handler = fn;
    irq_arg = arg;

    if (fn && (irq != OFF))
    {
        if (!has_irqs)
        {
            PO_INFO("gpio_install_isr_service()");
            const int intr_alloc_flags = 0; // TODO : check this
            err = gpio_install_isr_service(intr_alloc_flags);
            esp_check(err, __LINE__);
            has_irqs = true;
        }

        err = gpio_isr_handler_add((gpio_num_t) pin, gpio_irq_handler, this);
        esp_check(err, __LINE__);

        gpio_int_type_t type = GPIO_INTR_DISABLE;

        switch (irq)
        {
            case OFF    : type = GPIO_INTR_DISABLE; break;
            case RISE   : type = GPIO_INTR_POSEDGE; break;
            case FALL   : type = GPIO_INTR_NEGEDGE; break;
            case CHANGE : type = GPIO_INTR_ANYEDGE; break;
            default : ASSERT(0);
        }
 
        err = gpio_set_intr_type((gpio_num_t) pin, type);
        esp_check(err, __LINE__);
    }
    else
    {
        err = gpio_isr_handler_remove((gpio_num_t) pin);
        esp_check(err, __LINE__);
    }
}

void ESP_GPIO::on_interrupt()
{
    ASSERT(irq_handler);
    irq_handler(irq_arg);
}

}   //  namepace panglos

#endif  //  ESP32

//  FIN
