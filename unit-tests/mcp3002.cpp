
#include <gtest/gtest.h>

#include "panglos/debug.h"

#include "panglos/drivers/spi.h"
#include "panglos/drivers/mcp3002.h"

#include "mock.h"

using namespace panglos;

class TestSpiAdc : public SPI
{
    uint8_t cmd[2];
    uint16_t result;
    SPI::Lock cs_lock;
public:
    TestSpiAdc()
    :   cs_lock(0)
    {
    }
    void set_result(uint16_t r)
    {
        result = r;
    }
    virtual bool write(const uint8_t *data, int size) override
    {
        IGNORE(data);
        IGNORE(size);
        ASSERT(0);
        return true;
    }
    virtual bool read(uint8_t *data, int size) override
    {
        IGNORE(data);
        IGNORE(size);
        ASSERT(0);
        return true;
    }
    virtual bool io(const uint8_t *data, uint8_t *rd, int size) override
    {
        EXPECT_EQ(2, size);
        memcpy(cmd, data, size_t(size));
        rd[0] = uint8_t(result >> 8);
        rd[1] = uint8_t(result & 0xff);
        return true;
    }
    virtual void set_blocking(bool) override
    {
    }
    virtual uint32_t set_speed(uint32_t hz) override
    {
        return hz;
    }
    virtual Lock *lock() override
    {
        return & cs_lock;
    }
};

TEST(SpiAdc, Test)
{
    MockPin cs(0);
    TestSpiAdc spi;
    SpiDevice dev(& spi, & cs, 0);
    MCP3002 adc(& dev);

    spi.set_result(0x123);
    uint16_t v = adc.read(0);
    EXPECT_EQ(v, 0x123);
}

//  FIN
