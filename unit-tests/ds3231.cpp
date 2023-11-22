
#include <gtest/gtest.h>

#include "panglos/debug.h"

#include "panglos/mutex.h"
#include "panglos/drivers/i2c.h"
#include "panglos/drivers/ds3231.h"

using namespace panglos;

class Test_I2C : public I2C
{
public:
    Test_I2C()
    :   I2C(0)
    {
    }

    virtual bool probe(uint8_t addr, uint32_t timeout)
    {
        ASSERT(addr == DS3231::ADDR);
        IGNORE(timeout);
        return true;
    }

    virtual int write(uint8_t addr, const uint8_t* wr, uint32_t len)
    {
        ASSERT(addr == DS3231::ADDR);
        EXPECT_EQ(len, 8); // reg=0,s,m,h,x,d,m,y regs
        EXPECT_EQ(wr[1], 0x23);
        EXPECT_EQ(wr[2], 0x15);
        EXPECT_EQ(wr[3], 0);
        EXPECT_EQ(wr[5], 0x15);
        EXPECT_EQ(wr[6], 0x01);
        EXPECT_EQ(wr[7], 0x40);
        return int(len);
    }

    virtual int write_read(uint8_t addr, const uint8_t* wr, uint32_t len_wr, uint8_t* rd, uint32_t len_rd)
    {
        ASSERT(addr == DS3231::ADDR);
        ASSERT(*wr == 0);
        ASSERT(len_wr == 1);
        EXPECT_EQ(len_rd, 7); // fetch the s,m,h,x,d,m,y regs
        rd[0] = 0x12; // s
        rd[1] = 0x59; // m
        rd[2] = 0x23; // h
        rd[4] = 0x19; // d
        rd[5] = 0x11; // m
        rd[6] = 0x22; // y
        return int(len_rd);
    }

    virtual int read(uint8_t addr, uint8_t* rd, uint32_t len)
    {
        IGNORE(addr);
        IGNORE(rd);
        ASSERT(0);
        return int(len);
    }
};

    /*
     *
     */

TEST(DS3231, BCD)
{
    struct Pairs {
        uint8_t a;
        uint8_t b;
    };

    const struct Pairs test[] = {
        {   0, 0, }, {   0x01, 1, }, {   0x09, 9, },
        {   0x10, 10, }, {   0x12, 12, }, {   0x19, 19, },
        {   0x20, 20, }, {   0x29, 29, },
        {   0x30, 30, }, {   0x39, 39, },
        {   0x40, 40, }, {   0x49, 49, },
        {   0x50, 50, }, {   0x59, 59, },
        {   0x60, 60, }, {   0x69, 69, },
        {   0x70, 70, }, {   0x79, 79, },
        {   0x80, 80, }, {   0x89, 89, },
        {   0x90, 90, }, {   0x99, 99, },
        {   0xff, 0, },
    };

    for (const struct Pairs *p = test; p->a != 0xff; p++)
    {
        EXPECT_EQ(DS3231::from_bcd(p->a), p->b);
        EXPECT_EQ(p->a, DS3231::to_bcd(p->b));
    }
}

    /*
     *
     */

TEST(DS3231, Init)
{
    Test_I2C i2c;

    DS3231 rtc(& i2c);

    const bool ok = rtc.init();
    EXPECT_TRUE(ok);
}

    /*
     *
     */

TEST(DS3231, Get)
{
    Test_I2C i2c;

    DS3231 rtc(& i2c);

    rtc.init();

    RTC::DateTime dt;
    const bool ok = rtc.get(& dt);
    EXPECT_TRUE(ok);

    EXPECT_EQ(dt.sec, 12);
    EXPECT_EQ(dt.min, 59);
    EXPECT_EQ(dt.hour, 23);
    EXPECT_EQ(dt.day, 19);
    EXPECT_EQ(dt.month, 11);
    EXPECT_EQ(dt.year, 2022);
}

    /*
     *
     */

TEST(DS3231, Set)
{
    Test_I2C i2c;

    DS3231 rtc(& i2c);

    rtc.init();

    RTC::DateTime dt = {
        .sec = 23,
        .min = 15,
        .hour = 0,
        .day = 15,
        .month = 1,
        .year = 2040,
    };
    const bool ok = rtc.set(& dt);
    EXPECT_TRUE(ok);
}

//  FIN
