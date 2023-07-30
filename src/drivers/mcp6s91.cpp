
#include "panglos/debug.h"

#include "panglos/drivers/spi.h"
#include "panglos/drivers/mcp6s91.h"

namespace panglos {

enum CMD {
    NOP         = 0x00,
    SHUTDOWN    = 0x20,
    WRITE       = 0x40,
};

MCP6S9x::MCP6S9x(SpiDevice *_spi)
:   spi(_spi)
{
}

uint8_t MCP6S9x::set_gain(uint8_t gain)
{
    ASSERT(gain);

    struct Gain { uint8_t code; uint8_t gain; };

    static const Gain gains[] = {
        {   7, 32 },
        {   6, 16 },
        {   5, 10 },
        {   4, 8 },
        {   3, 5 },
        {   2, 4 },
        {   1, 2 },
        {   0, 1 },
        {   0, 0 },
    };

    for (int i = 0; gains[i].gain; i++)
    {
        if (gain >= gains[i].gain)
        {
            uint8_t data[] = { 
                WRITE + 0,
                gains[i].code,
            };
            spi->write(data, sizeof(data));
            return gains[i].gain;
        }
    }

    PO_ERROR("gain=%d", gain);
    return 0;
}

void MCP6S9x::set_chan(uint8_t chan)
{
    uint8_t data[] = { 
        WRITE + 1,
        chan,
    };
    spi->write(data, sizeof(data));
}

}   //  namespace panglos 

//  FIN
