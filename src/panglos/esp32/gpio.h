
#include "panglos/gpio.h"

namespace panglos {

class ESP_GPIO : public panglos::GPIO
{
    int pin;
    bool output;
    bool state;
    bool verbose;
    const char *name;

    void (*irq_handler)(void *arg);
    void *irq_arg;
public:

    enum Mode {
        IP = 1 << 0,
        OP = 1 << 1,
        OD = 1 << 2,
        PU = 1 << 3,
        PD = 1 << 4,
    };

    ESP_GPIO(int _pin, int mode, bool initial_state = false, bool verbose=false, const char *name=0);

    virtual ~ESP_GPIO();

    virtual void set(bool _state) override;
    virtual bool get() override;

    virtual void set_interrupt_handler(enum Interrupt irq, void (*fn)(void *arg), void *arg) override;
    virtual void on_interrupt() override;

    static void mark_used(int pin);
    static void mark_unused(int pin);
};

}   //  namespace panglos

// FIN
