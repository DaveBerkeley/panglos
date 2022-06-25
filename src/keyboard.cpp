    /*
     *
     */

#include <stdint.h>

#include "panglos/debug.h"
#include "panglos/time.h"

#include "panglos/keyboard.h"

namespace panglos {

Keyboard::Keyboard(MCP23S17 *_dev, GPIO *_irq)
:   dev(_dev),
    irq(_irq),
    leds(0xff),
    on_key(0),
    on_key_arg(0)
{
    ASSERT(dev);

    if (irq)
    {
        irq->set_interrupt_handler(on_interrupt, this);
    }
}

Keyboard::~Keyboard()
{
    if (irq)
    {
        irq->set_interrupt_handler(0, 0);
    }
}

    /*
     *
     */

void Keyboard::set_key_event(void (*fn)(void *arg), void *arg)
{
    on_key = fn;
    on_key_arg = arg;
}

void Keyboard::on_interrupt()
{
    if (!on_key)
    {
        PO_ERROR("no handler set");
        return;
    }

    on_key(on_key_arg);
}

void Keyboard::on_interrupt(void *arg)
{
    ASSERT(arg);
    Keyboard *kb = (Keyboard *) arg;
    kb->on_interrupt();
}

    /*
     *
     */

bool Keyboard::init(int nkeys)
{
    ASSERT(nkeys);
    ASSERT(nkeys <= 8);

    const uint8_t mask = (uint8_t)((1 << nkeys) - 1);
    ASSERT(mask);

    // LEDs on port A outputs
    // Keys on port B inputs
    dev->write(MCP23S17::R_IODIRA, mask); // key inputs
    dev->write(MCP23S17::R_GPPUA, mask); // pull-up
    dev->write(MCP23S17::R_GPIOB, leds); // leds all off
    dev->write(MCP23S17::R_IODIRB, 0x00); // led outputs

    dev->write(MCP23S17::R_IOCON, 0x00); // bank=0, mirror=0, odr=0, intpol=0, seqop=0

    if (irq)
    {
        // Configure to generate interrupts on key change
        dev->write(MCP23S17::R_GPINTENB, mask); // enable interrupt on change for inputs
        dev->write(MCP23S17::R_INTCONB, 0); // 0=compare to previous pin value for change
    }

    return true;
}

    /*
     *
     */

void Keyboard::set_led(int idx, bool state)
{
    //PO_DEBUG("idx=%d state=%d", idx, state);
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

    uint8_t d = 0;
    const bool ok = dev->read(MCP23S17::R_GPIOB, & d);
    if (!ok) PO_ERROR("");
    return d & mask;
}

uint8_t Keyboard::read_keys()
{
    uint8_t d = 0;
    const bool ok = dev->read(MCP23S17::R_GPIOA, & d);
    if (!ok) PO_ERROR("");
    return ok ? d : 0;
}

}   //  namespace panglos

//  FIN
