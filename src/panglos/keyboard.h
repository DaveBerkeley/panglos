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
    uint8_t leds;
public:
    Keyboard(MCP23S17 *_dev);

    bool init();
    void set_led(int idx, bool state);
    bool get_led(int idx);

    uint8_t read_keys();

    void tick();
};

}   //  namespace panglos

#endif  //  __PANGLOS_KEYBOARD__

//  FIN
