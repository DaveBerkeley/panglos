    /*
     *
     */

#include <stdint.h>

#include "panglos/debug.h"

#include "panglos/keyboard.h"

namespace panglos {

Keyboard::Keyboard(MCP23S17 *_dev)
:   dev(_dev),
    leds(0xff)
{
    ASSERT(dev);
}

bool Keyboard::init()
{
    dev->write(MCP23S17::R_IODIRA, 0xff); // key input
    dev->write(MCP23S17::R_GPPUA, 0xff); // pull-up
    dev->write(MCP23S17::R_GPIOB, leds); // leds all off
    dev->write(MCP23S17::R_IODIRB, 0x00); // led output
    return true;
}

void Keyboard::set_led(int idx, bool state)
{
    ASSERT((idx >= 0) && (idx <= 8));
    const uint8_t mask = (uint8_t) (1 << idx);

    if (!state)
    {
        leds |= mask;
    }
    else
    {
        leds &= (uint8_t) ~mask;
    }
    dev->write(MCP23S17::R_GPIOB, (uint8_t) ~leds); 
}

bool Keyboard::get_led(int idx)
{
    ASSERT((idx >= 0) && (idx <= 8));
    const uint8_t mask = (uint8_t) (1 << idx);

    const uint8_t d = dev->read(MCP23S17::R_GPIOB);
    return d & mask;        
}

uint8_t Keyboard::read_keys()
{
    const uint8_t d = dev->read(MCP23S17::R_GPIOA);
    return d;
}

void Keyboard::tick()
{
    // TODO
    ASSERT(0);
}

}   //  namespace panglos

//  FIN
