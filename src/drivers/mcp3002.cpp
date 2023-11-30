
#include "panglos/debug.h"

#include "panglos/drivers/spi.h"
#include "panglos/drivers/mcp3002.h"

namespace panglos {

MCP3002::MCP3002(SpiDevice *_spi)
:   spi(_spi)
{
}

uint16_t MCP3002::read(uint8_t chan)
{
    ASSERT(chan < 2);
    const uint8_t mode = 0x68; // 0, START, SINGLE, (chan_x), MSBF, 0, 0, 0
    uint8_t cmd[] = {
        uint8_t(mode + (chan << 4)),
        0,
    };
    uint8_t rd[2];

    const bool ok = spi->io(cmd, rd, 2);
    ASSERT(ok);
    const uint16_t adc = uint16_t(((rd[0] & 0x3) << 8) + rd[1]);
    return adc;
}

}   //  namespace panglos

//  FIN
