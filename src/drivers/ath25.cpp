
#include <stdint.h>

#include "panglos/debug.h"

#include "panglos/mutex.h"
#include "panglos/drivers/i2c.h"
#include "panglos/drivers/ath25.h"

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

const uint8_t ATH25::ADDR = 0x38;

ATH25::ATH25(I2C *_i2c)
:   i2c(_i2c)
{
    ASSERT(i2c);
}

bool ATH25::probe(I2C *i2c)
{
    ASSERT(i2c);
    return i2c->probe(ADDR, 2);
}

void ATH25::init()
{
    uint8_t cmd[] = { CMD_INIT, };
    i2c->write(ADDR, cmd, sizeof(cmd));
}

Time::tick_t ATH25::request()
{
    uint8_t cmd[] = { CMD_REQ, };
    i2c->write(ADDR, cmd, sizeof(cmd));
    return 80; // ms to wait
}

bool ATH25::get(Reading *r)
{
    uint8_t data[7];

    int n = i2c->read(ADDR, data, sizeof(data));
    if (n != sizeof(data))
    {
        return false;
    }

    const uint8_t state = data[0];
    if (state & S_BUSY)
    {
        return false;
    }

    ASSERT(r);

    // Assuming 20-bit data sent hi bit first
    int h = (data[1] << 12) + (data[2] << 4) + (data[3] >> 4);
    int  t = ((data[3] & 0x0f) << 12) + (data[4] << 4) + data[5];
    //uint8_t crc = data[6];
    // TODO : validate crc

    const double p20 = 1048576.0; // 2^20
    r->humidity = 100 * (h / p20);
    r->temperature = (200 * (t / p20)) - 50;
    return true;
}

}   //  namespace panglos

//  FIN
