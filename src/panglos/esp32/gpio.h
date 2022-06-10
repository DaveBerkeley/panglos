
#include "panglos/gpio.h"

namespace panglos {

class ESP_GPIO : public panglos::GPIO
{
    gpio_num_t pin;
    bool output;
    bool state;
    bool verbose;
public:
    ESP_GPIO(gpio_num_t _pin, bool input, bool pullup, bool pulldown, bool initial_state = false, bool verbose=false);

    virtual ~ESP_GPIO();

    virtual void set(bool _state) override;
    virtual bool get() override;
    virtual void toggle() override;

    virtual void set_interrupt_handler(void (*fn)(void *arg), void *arg) override;
    virtual void on_interrupt() override;
};

}   //  namespace panglos

// FIN
