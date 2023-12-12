
#include <stdlib.h>

#include "panglos/debug.h"
#include "panglos/mutex.h"

#include "panglos/drivers/i2c.h"
#include "panglos/drivers/amg88xx.h"

namespace panglos {

AMG88xx::AMG88xx(enum Addr _addr, I2C *_i2c)
:   i2c(_i2c),
    addr(_addr)
{
    PO_DEBUG("");
}

bool AMG88xx::read_frame(uint8_t *data, size_t size)
{
    uint8_t cmd = Pixel;
    const int n = i2c->write_read(addr, & cmd, sizeof(cmd), data, size);
    return n == (size + sizeof(cmd));
}

bool AMG88xx::probe()
{
    return i2c->probe(addr, 2);
}

void AMG88xx::init()
{
    {
        uint8_t cmd[] = { Reset, 0x3f };
        i2c->write(addr, cmd, sizeof(cmd));
    }

    struct Pair {
        uint8_t reg;
        uint8_t data;
    };

    struct Pair pairs[] = {
        {   PowerControl, 0,    }, // 00= Normal Mode
        {   FrameRate, 0,    }, // 00= 10fps
    };

    for (int i = 0; i < (sizeof(pairs) / sizeof(pairs[0])); i++)
    {
        struct Pair *pair = & pairs[i];
        i2c->write(addr, & pair->reg, sizeof(*pair));
    }
}

}   //  namespace panglos

//  FIN
