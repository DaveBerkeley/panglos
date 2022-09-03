
#include <gtest/gtest.h>

#include "panglos/debug.h"

#include "panglos/mutex.h"
#include "panglos/drivers/i2c.h"
#include "panglos/drivers/aht25.h"

using namespace panglos;

class AHT25_I2C : public I2C
{
public:
    uint8_t written[10];
    uint8_t reads[10];

    AHT25_I2C()
    :   I2C(0)
    {
        memset(written, 0, sizeof(written));
        memset(reads, 0, sizeof(reads));
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
        ASSERT(len < sizeof(written));
        memcpy(written, wr, len);
        return int(len);
    }
    virtual int write_read(uint8_t addr, const uint8_t* wr, uint32_t len_wr, uint8_t* rd, uint32_t len_rd) override
    {
        EXPECT_EQ(addr, AHT25::ADDR);
        ASSERT(len_wr < sizeof(written));
        memcpy(written, wr, len_wr);
        ASSERT(rd);
        ASSERT(len_rd <= sizeof(reads));
        memcpy(rd, reads, len_rd);
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
    AHT25 dut(& i2c, true);

    i2c.reads[0] = 0xc8; // ready
    bool ok = dut.init();
    EXPECT_TRUE(ok);

    int p = dut.request();
    EXPECT_EQ(AHT25::CMD_REQ, i2c.written[0]);
    EXPECT_EQ(80, p);
}

//  FIN
