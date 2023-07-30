
#include <gtest/gtest.h>

#include "panglos/debug.h"

#include "panglos/drivers/mcp6s91.h"

#include "mock.h"

using namespace panglos;

class TestSpi : public SPI
{
public:
    uint8_t gain;
    uint8_t chan;

    TestSpi()
    :   SPI(0)
    {
        reset();
    }

    void reset()
    {
        gain = 0xff;
        chan = 0xff;
    }

    virtual bool write(const uint8_t *data, int size) override
    {
        ASSERT(size == 2);
        const uint8_t cmd = data[0] & 0xfe;
        ASSERT(cmd == 0x40);
        if (data[0] & 0x01)
        {
            chan = data[1];
        }
        else
        {
            gain = data[1];
        }
        return true;
    }

    virtual bool io(const uint8_t *data, uint8_t *rd, int size) override
    {
        IGNORE(data);
        IGNORE(rd);
        IGNORE(size);
        ASSERT(0);
        return false;
    }
};

TEST(PGA, Chan)
{
    MockPin cs(0);
    TestSpi spi;
    SpiDevice dev(& spi, & cs, 0);
    MCP6S9x pga(& dev);

    spi.reset();
    pga.set_chan(0);
    EXPECT_EQ(spi.chan, 0);

    spi.reset();
    pga.set_chan(1);
    EXPECT_EQ(spi.chan, 1);
}

TEST(PGA, Gain)
{
    MockPin cs(0);
    TestSpi spi;
    SpiDevice dev(& spi, & cs, 0);
    MCP6S9x pga(& dev);

    uint8_t gains[][3] = {
        {   1, 0, 1 },
        {   2, 1, 2 },
        {   3, 1, 2 },
        {   4, 2, 4 },
        {   5, 3, 5 },
        {   6, 3, 5 },
        {   7, 3, 5 },
        {   8, 4, 8 },
        {   9, 4, 8 },
        {  10, 5, 10 },
        {  11, 5, 10 },
        {  12, 5, 10 },
        {  13, 5, 10 },
        {  14, 5, 10 },
        {  15, 5, 10 },
        {  16, 6, 16 },
        {  17, 6, 16 },
        {  18, 6, 16 },
        {  19, 6, 16 },
        {  30, 6, 16 },
        {  31, 6, 16 },
        {  32, 7, 32 },
        {  33, 7, 32 },
        {  34, 7, 32 },
        {  35, 7, 32 },
        {  36, 7, 32 },
        {  37, 7, 32 },
        {  38, 7, 32 },
        { 100, 7, 32 },
        { 0, 0, 0  },
    };

    for (int i = 0; gains[i][0]; i++)
    {
        spi.reset();
        uint8_t r = pga.set_gain(gains[i][0]);
        EXPECT_EQ(spi.gain, gains[i][1]);
        EXPECT_EQ(r, gains[i][2]);        
    }
}

//  FIN
