
#include <gtest/gtest.h>

#include "panglos/debug.h"

#include "panglos/mutex.h"
#include "panglos/drivers/i2c.h"
#include "panglos/drivers/aht25.h"

using namespace panglos;

class AHT25_I2C : public I2C
{
public:
    uint8_t written;

    AHT25_I2C()
    :   I2C(0),
        written(0)
    {
    }

    virtual bool probe(uint8_t addr, uint32_t timeout) override
    {
        EXPECT_EQ(addr, AHT25::ADDR);
        IGNORE(timeout);
        return true;
    }
    virtual int write(uint8_t addr, const uint8_t* wr, uint32_t len) override
    {
        EXPECT_EQ(addr, AHT25::ADDR);
        EXPECT_EQ(1, len);
        written = *wr;
        return int(len);
    }
    virtual int write_read(uint8_t addr, const uint8_t* wr, uint32_t len_wr, uint8_t* rd, uint32_t len_rd) override
    {
        ASSERT(0);
        EXPECT_EQ(addr, AHT25::ADDR);
        IGNORE(wr);
        IGNORE(len_wr);
        IGNORE(rd);
        IGNORE(len_rd);
        return int(len_wr);
    }
    virtual int read(uint8_t addr, uint8_t* rd, uint32_t len) override
    {
        const uint8_t data[] = {
            0x12, 0x34, 0x45, 0x56, 0x67, 0x78, 0x89,
        };
        EXPECT_EQ(addr, AHT25::ADDR);
        EXPECT_EQ(sizeof(data), len);
        memcpy(rd, data, len);
        return int(len);
    }
};

TEST(AHT25, Test)
{
    AHT25_I2C i2c;
    AHT25 dut(& i2c);

    bool ok;

    ok = dut.probe(& i2c);
    EXPECT_TRUE(ok);
    EXPECT_EQ(0, i2c.written);

    dut.init();
    EXPECT_EQ(AHT25::CMD_INIT, i2c.written);

    Time::tick_t p = dut.request();
    EXPECT_EQ(AHT25::CMD_REQ, i2c.written);
    EXPECT_EQ(80, p);

    AHT25::Reading r;
    ok = dut.get(& r);

    PO_INFO("TODO : check readings");
    PO_INFO("TODO : check crc");
}

//  FIN
