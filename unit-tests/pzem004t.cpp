
#include <gtest/gtest.h>

#include "panglos/debug.h"
#include "panglos/io.h"

#include "panglos/drivers/pzem004t.h"

using namespace panglos;

TEST(PZEM, CRC)
{
    const uint8_t data[] = {  0x12, 0x23, 0x34, 0x56, };
    uint16_t crc = PZEM004T::crc(data, sizeof(data));
    EXPECT_EQ(crc, 26722); 
}

TEST(PZEM, Request)
{
    char buffer[128] = { 0 };
    CharOut out(buffer, sizeof(buffer));
    PZEM004T dut(& out);

    bool ok = dut.request();
    EXPECT_TRUE(ok);
    // check the buffer
    EXPECT_EQ(buffer[0], 0x01);
    EXPECT_EQ(buffer[1], 0x04);
    EXPECT_EQ(buffer[2], 0);
    EXPECT_EQ(buffer[3], 0);
    EXPECT_EQ(buffer[4], 0);
    EXPECT_EQ(buffer[5], '\n');
    EXPECT_EQ(buffer[6], 'p');
    EXPECT_EQ(buffer[7], '\r');
}

TEST(PZEM, Parse)
{
    PZEM004T dut(0);

    bool ok;

    {
        // Bad data
        const uint8_t data[] = { 0x01, 0x02, 0x03 };
        ok = dut.parse(0, data, sizeof(data));
        EXPECT_FALSE(ok);
    }
    {
        struct PZEM004T::Status status;
        //v=248.5 i=0.0 p=0.0 e=680.0 f=49.900000000000006 pf=0.0 alarm=0
        const uint8_t data[] = "\x01\x04\x14\t\xb5\x00\x00\x00\x00\x00\x00\x00\x00\x02\xa8\x00\x00\x01\xf3\x00\x00\x00\x00\xd8\xc6";
        ok = dut.parse(& status, data, sizeof(data));
        EXPECT_TRUE(ok);
        EXPECT_FLOAT_EQ(float(248.5), status.volts);
        EXPECT_FLOAT_EQ(0.0, status.current);
        EXPECT_FLOAT_EQ(0.0, status.power);
        EXPECT_FLOAT_EQ(float(680.0), status.energy);
        EXPECT_FLOAT_EQ(float(49.9), status.freq);
        EXPECT_FLOAT_EQ(0.0, status.power_factor);
    }
}

//  FIN
