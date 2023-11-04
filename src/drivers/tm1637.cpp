
#include <stdint.h>
#include <stdio.h>

#include "panglos/debug.h"

#include "panglos/mutex.h"
#include "panglos/drivers/gpio.h"

#include "panglos/drivers/tm1637.h"

namespace panglos {

enum Command {
    DATA = 0x40,
    DISP = 0x80,
    ADDR = 0xC0,
};

TM1637::TM1637(TwoWire *_tw)
:   tw(_tw)
{
    ASSERT(tw);
}

void TM1637::write(const char *text)
{
    PO_DEBUG("'%s'", text);

    char buff[5] = { 0 };

    snprintf(buff, sizeof(buff), "%*s", 4, text);

    uint8_t data[5] = { ADDR + (0), };

    for (int i = 0; i < 4; i++)
    {            
        data[1+i] = SevenSegment::seg(buff[i]);
    }

    tw->write(0, data, sizeof(data));
}

bool TM1637::command(uint8_t cmd, uint8_t data)
{
    uint8_t d[] = { cmd, data };
    return tw->write(0, d, sizeof(d)) == 2;
}

bool TM1637::init()
{
    uint8_t cmd = DATA + (0); // wr display, auto addr inc, normal mode
    if (tw->write(0, & cmd, sizeof(cmd)) != sizeof(cmd))
    {
        return false;
    }

    write("    ");

    cmd = DISP + (0xf); // brightness
    if (tw->write(0, & cmd, sizeof(cmd)) != sizeof(cmd))
    {
        return false;
    }

    return true;
}

}   //  namespace panglos

