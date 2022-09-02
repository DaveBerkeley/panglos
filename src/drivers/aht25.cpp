
#include <stdint.h>

#include "panglos/debug.h"

#include "panglos/mutex.h"
#include "panglos/drivers/i2c.h"
#include "panglos/drivers/aht25.h"

namespace panglos {

    /*
     *  See http://www.aosong.com/en/products-61.html
     */

// Status Bits
#define S_BUSY 0x80
#define S_CAL  0x08

    /*
     *
     */

const uint8_t AHT25::ADDR = 0x38;

AHT25::AHT25(I2C *_i2c)
:   i2c(_i2c)
{
    ASSERT(i2c);
}

bool AHT25::probe(I2C *i2c)
{
    ASSERT(i2c);
    return i2c->probe(ADDR, 2);
}

bool AHT25::init()
{
    {
        uint8_t cmd[] = { CMD_RESET, };
        i2c->write(ADDR, cmd, sizeof(cmd));
    }

    Time::msleep(40);

    uint8_t cmd[] = { CMD_INIT, 0x80, 0x00, };
    i2c->write(ADDR, cmd, sizeof(cmd));

    for (int i = 0; i < 10; i++)
    {
        if (calibrated())
        {
            return true;
        }

        Time::msleep(10);
    }

    return false;
}

Time::tick_t AHT25::request()
{
    uint8_t cmd[] = { CMD_REQ, 0x33, 0, };
    i2c->write(ADDR, cmd, sizeof(cmd));
    // TODO : convert to ticks
    return 8; // ms to wait
    return 80; // ms to wait
}

uint8_t AHT25::status()
{
    uint8_t cmd[] = { CMD_STATUS, };
    uint8_t result;
    i2c->write_read(ADDR, cmd, sizeof(cmd), & result, sizeof(result));
    return result;
}

bool AHT25::busy()
{
    return (status() & S_BUSY) == S_BUSY;
}

bool AHT25::calibrated()
{
    return (status() & S_CAL) == S_CAL;
}

bool AHT25::get(Reading *r)
{
    uint8_t data[7] = { 0, };

    int n = i2c->read(ADDR, data, sizeof(data));
    if (n != sizeof(data))
    {
        PO_ERROR("");
        return false;
    }

    //PO_DEBUG("%#02x %#02x %#02x %#02x %#02x %#02x %#02x",
    //        data[0], data[1], data[2], data[3], data[4], data[5], data[6]);

    const uint8_t state = data[0];
    if (state & S_BUSY)
    {
        //PO_ERROR("state=%#x", state);
        return false;
    }

    ASSERT(r);

    // Assuming 20-bit data sent hi bit first
    int h = (data[1] << 12) + (data[2] << 4) + (data[3] >> 4);
    int t = ((data[3] & 0x0f) << 16) + (data[4] << 8) + data[5];
    //PO_DEBUG("h=%#x t=%#x", h, t);
    //uint8_t crc = data[6];
    // TODO : validate crc

    const double p20 = 1048576.0; // 2^20
    r->humidity = 100 * (h / p20);
    r->temperature = (200 * (t / p20)) - 50;
    return true;
}

}   //  namespace panglos

//  FIN
