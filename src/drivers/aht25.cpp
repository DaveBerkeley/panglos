
#include <stdint.h>

#include "panglos/debug.h"

#include "panglos/mutex.h"
#include "panglos/drivers/i2c.h"
#include "panglos/drivers/aht25.h"

namespace panglos {

    /*
     *  See http://www.aosong.com/en/products-61.html
     */

const uint8_t AHT25::ADDR = 0x38;

// Status Bits
#define S_BUSY 0x80
#define S_CAL  0x08

    /*
     *
     */

uint8_t AHT25::crc8(uint8_t *data, int n)
{
    uint8_t crc = 0xff;
    for (int i = 0; i < n; i++)
    {
        crc ^= data[i];
        for (int j = 0; j < 8; j++)
        {
            if (crc & 0x80)
            {
                crc = uint8_t((crc << 1) ^0x31);
            }
            else
            {
                crc = uint8_t(crc << 1);
            }
        }
    }
    return crc;
}

    /*
     *
     */

AHT25::AHT25(I2C *_i2c, bool _verbose)
:   i2c(_i2c),
    verbose(_verbose)
{
    ASSERT(i2c);
}

bool AHT25::init()
{
    {
        uint8_t cmd[] = { CMD_RESET, };
        if (!i2c->write(ADDR, cmd, sizeof(cmd)))
        {
            return false;
        }
    }

    Time::msleep(40);

    uint8_t cmd[] = { CMD_INIT, 0x00, 0x00, };
    i2c->write(ADDR, cmd, sizeof(cmd));

    for (int i = 0; i < 10; i++)
    {
        if (calibrated())
        {
            return true;
        }

        Time::msleep(10);
    }

    if (verbose) PO_ERROR("failed init");
    return false;
}

int AHT25::request()
{
    uint8_t cmd[] = { CMD_REQ, 0x33, 0, };
    i2c->write(ADDR, cmd, sizeof(cmd));
    return 80; // ms to wait
}

uint8_t AHT25::status()
{
    uint8_t cmd[] = { CMD_STATUS, };
    uint8_t result = 0;
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
        if (verbose) PO_ERROR("");
        return false;
    }

    const uint8_t state = data[0];
    if (state & S_BUSY)
    {
        if (verbose) PO_ERROR("busy");
        return false;
    }

    ASSERT(r);

    // 20-bit data sent hi bit first
    const int h = (data[1] << 12) + (data[2] << 4) + (data[3] >> 4);
    const int t = ((data[3] & 0x0f) << 16) + (data[4] << 8) + data[5];

    if (data[6] != crc8(data, 6))
    {
        // Bad CRC
        if (verbose) PO_ERROR("bad crc");
        return false;
    }

    const double p20 = 1048576.0; // 2^20
    r->humidity = 100 * (h / p20);
    r->temperature = (200 * (t / p20)) - 50;
    return true;
}

}   //  namespace panglos

//  FIN
