
#include <stdint.h>

#include "panglos/debug.h"

#include "panglos/drivers/spi.h"
#include "panglos/drivers/max7219.h"

namespace panglos {

MAX7219::MAX7219(SpiDevice *_spi)
:   spi(_spi)
{
    ASSERT(spi);
}

bool MAX7219::init()
{
    PO_DEBUG("");
    bool ok = true;
    ok &= write(SHUTDOWN, 0x01);      // normal operation
    ok &= write(DECODE_MODE, 0x00);   // no decode (raw 7-seg)
    ok &= write(SCAN_LIMIT, 0x07);    // all digits
    set_brightness(0xff);
    return ok;
}

bool MAX7219::write(enum Reg reg, uint8_t value)
{
    uint8_t data[] = { reg, value };
    return spi->write(data, sizeof(data));
}

void MAX7219::write(const char *text)
{
    ASSERT(text);
    uint8_t addr = DIGIT7;

    for (int i = 0; i < 8; i++)
    {
        const char c = *text ? *text : ' ';
        write((enum Reg) addr, SevenSegment::seg(c, true));
        if (*text) text++;
        addr -= 1;
        if (addr < DIGIT0) break;
    }
}

void MAX7219::set_brightness(uint8_t level)
{
    write(INTENSITY, level >> 4);
}

}   //  namespace panglos

//  FIN
