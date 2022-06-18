    /*
     *
     */

#if !defined(__PANGLOS_KEYBOARD__)
#define __PANGLOS_KEYBOARD__

#include "panglos/mcp23s17.h"

namespace panglos {

class Keyboard
{
    MCP23S17 *dev;
    GPIO *irq;
    uint8_t leds;

    void (*on_key)(void *arg);
    void *on_key_arg;

    static void on_interrupt(void *arg);
    void on_interrupt();

public:
    Keyboard(MCP23S17 *_dev, GPIO *irq);
    ~Keyboard();

    bool init(int nkeys=8);
    void set_led(int idx, bool state);
    bool get_led(int idx);

    uint8_t read_keys();

    void set_key_event(void (*fn)(void *arg), void *arg);
};

}   //  namespace panglos

#endif  //  __PANGLOS_KEYBOARD__

//  FIN
