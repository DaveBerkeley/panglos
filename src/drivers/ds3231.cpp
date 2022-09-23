
#include "panglos/debug.h"

#include "panglos/mutex.h"
#include "panglos/drivers/i2c.h"
#include "panglos/drivers/ds3231.h"

#define REG_SEC 0
//#define REG_MIN 1
//#define REG_HR  2
//#define REG_DAY 3
//#define REG_DATE 4
//#define REG_MON 5
//#define REG_YR 6
#define REG_CONTROL 0x0e
#define REG_STATUS 0x0f
#define REG_AGING 0x10
#define REG_TEMP_HI 0x11
#define REG_TEMP_LO 0x12

namespace panglos {

uint8_t DS3231::from_bcd(uint8_t data)
{
    uint8_t lo = data & 0x0f;
    uint8_t hi = uint8_t(data >> 4);
    return uint8_t(lo + (10 * hi));
}

uint8_t DS3231::to_bcd(uint8_t data)
{
    uint8_t lo = data % 10;
    uint8_t hi = data / 10;
    return uint8_t(lo + (hi << 4));
}

    /*
     *
     */

const uint8_t DS3231::ADDR = 0x68; // TODO : check 7-bit

    /*
     *
     */

DS3231::DS3231(I2C *_i2c)
:   i2c(_i2c)
{
    IGNORE(i2c);
}

    /*
     *
     */

bool DS3231::init()
{
    ASSERT(i2c);
    if (!i2c->probe(ADDR, 2))
    {
        PO_INFO("No DS3231 found");
        return false;
    }

    // TODO : make sure it is in the right mode?
    //PO_WARNING("TODO : check RTC modes etc.");

    return true;
}

    /*
     *
     */

bool DS3231::set(const DateTime *dt)
{
    ASSERT(i2c);
    ASSERT(dt);

    const uint8_t wr[] = {
        REG_SEC,
        to_bcd(dt->sec),
        to_bcd(dt->min),
        to_bcd(dt->hour),
        0, // day of week, don't care
        to_bcd(dt->day),
        to_bcd(dt->month),
        to_bcd(uint8_t(dt->year - 2000)),
    };

    int n = i2c->write(ADDR, wr, sizeof(wr));
    return (n == sizeof(wr));
}

    /*
     *
     */

bool DS3231::get(DateTime *dt)
{
    ASSERT(i2c);
    ASSERT(dt);

    const uint8_t wr[] = { REG_SEC, };
    uint8_t rd[7];
    int n = i2c->write_read(ADDR, wr, sizeof(wr), rd, sizeof(rd));
    if (n != sizeof(rd))
    {
        PO_ERROR("n=%d", n);
        return false;
    }

    // decode data
    dt->sec   = from_bcd(rd[0]);
    dt->min   = from_bcd(rd[1]);
    dt->hour  = from_bcd(rd[2] & 0x3f); // assumes 24-hour mode
    dt->day   = from_bcd(rd[4] & 0x3f);
    dt->month = from_bcd(rd[5] & 0x1f);
    dt->year  = uint16_t(from_bcd(rd[6]) + 2000);

    return true;
}

}   //  namespace panglos

//  FIN
