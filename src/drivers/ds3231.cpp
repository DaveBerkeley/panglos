
#include <stdlib.h>

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

const uint8_t DS3231::ADDR = 0x68;

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

    const uint8_t wr[] = { 0 };
    uint8_t rd[7];

    if (!i2c->write_read(ADDR, wr, sizeof(wr), rd, sizeof(rd)))
    {
        PO_INFO("No DS3231 I2C RTC device found");
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
        to_bcd(uint8_t(dt->s)),
        to_bcd(uint8_t(dt->m)),
        to_bcd(uint8_t(dt->h)),
        0, // day of week, don't care
        to_bcd(uint8_t(dt->dd)),
        to_bcd(uint8_t(dt->mm)),
        to_bcd(uint8_t(dt->yy - 2000)),
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
    dt->s  = from_bcd(rd[0]);
    dt->m  = from_bcd(rd[1]);
    dt->h  = from_bcd(rd[2] & 0x3f); // assumes 24-hour mode
    dt->dd = from_bcd(rd[4] & 0x3f);
    dt->mm = from_bcd(rd[5] & 0x1f);
    dt->yy = uint16_t(from_bcd(rd[6]) + 2000);

    return true;
}

    /*
     *
     */

enum DS3231::Variant DS3231::get_variant()
{
    // The DS3231 has read-only temperature at 0x11, 0x12
    // The DS1307 has RAM from 0x08 to 0x30
    // Reg 0x11 is the uppoer 8 bit of the 10-bit temp
    // Reg 0x12 has the lower two bits at <<6, then 6 zero bits

    const uint8_t reg = 0x12;
    uint8_t rd_data;

    // Read the low byte of the temperature
    if (!i2c->write_read(ADDR, & reg, sizeof(reg), & rd_data, sizeof(rd_data))) return V_UNKNOWN;
    // if any of bits 0..5 are set, it must be RAM
    if (rd_data & 0x3f) return V_DS1307;

    // Try XORing the byte
    uint8_t wr_data[] = { reg, uint8_t(rd_data ^ 0xff) };
    if (!i2c->write(ADDR, wr_data, sizeof(wr_data))) return V_UNKNOWN;
    // read it again
    uint8_t read2;
    if (!i2c->write_read(ADDR, & reg, sizeof(reg), & read2, sizeof(read2))) return V_UNKNOWN;

    if (read2 != wr_data[1])
    {
        // not RAM therefore it must be a DS3231 with a read-only temperature sensor
        return V_DS3231;
    }

    // Must be RAM, which we've altered, so restore it
    wr_data[1] = rd_data;
    if (!i2c->write(ADDR, wr_data, sizeof(wr_data))) return V_UNKNOWN;

    return V_DS1307;
}

}   //  namespace panglos

//  FIN
