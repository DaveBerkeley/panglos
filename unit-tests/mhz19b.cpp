
#include <gtest/gtest.h>

#include "panglos/debug.h"

#include "panglos/drivers/mhz19b.h"

using namespace panglos;

class MHZ19B_Uart : public UART
{
public:
    char buff[9];
    uint16_t co2;

    virtual int rx(char* data, int n)
    {
        ASSERT(n == int(sizeof(buff)));
        ASSERT(data);
        memset(data, 0, n);
        data[0] = char(0xff);
        data[1] = char(0x86);
        data[2] = char(co2 >> 8);
        data[3] = char(co2 & 0xff);
        data[8] = MHZ19B::checksum((const uint8_t*) data);
        return n;
    }

    virtual int tx(const char* data, int n)
    {
        ASSERT(n <= int(sizeof(buff)));
        memcpy(buff, data, n);
        return n;
    }

    MHZ19B_Uart() : co2(0) { }
};

    /*
     *
     */

TEST(MHZ19B, Checksum)
{
    MHZ19B_Uart uart;
    MHZ19B dev(& uart);

    const uint8_t msg[] = {
        0xff, 0x01, 0x86, 0, 0, 0, 0, 0,
    };

    uint8_t cs = dev.checksum(msg);
    EXPECT_EQ(cs, 0x79);
}

TEST(MHZ19B, Request)
{
    MHZ19B_Uart uart;
    MHZ19B dev(& uart);

    bool ok;
    ok = dev.request();
    EXPECT_TRUE(ok);

    {
        const char msg[] = { char(0xff), 0x01, char(0x86), 0, 0, 0, 0, 0, 0x79, };
        EXPECT_EQ(0, memcmp(uart.buff, msg, sizeof(msg)));
    }
}

TEST(MHZ19B, Read)
{
    MHZ19B_Uart uart;
    MHZ19B dev(& uart);

    uint16_t test[] = {
        0,
        100,
        0x800,
        0xabcd,
    };

    for (size_t i = 0; i < (sizeof(test) / sizeof(test[0])); i++)
    {
        bool ok;
        ok = dev.request();
        EXPECT_TRUE(ok);

        uart.co2 = test[i];

        struct MHZ19B::Data data;
        ok = dev.read(& data);
        EXPECT_TRUE(ok);
        EXPECT_EQ(data.co2, test[i]);
    }
}

//  FIN
