
#include <stdint.h>

#include "panglos/debug.h"
#include "panglos/io.h"

#include "panglos/drivers/pzem004t.h"

namespace panglos {

PZEM004T::PZEM004T(Out *_out)
:   out(_out)
{
}

bool PZEM004T::request()
{
    ASSERT(out);

    uint8_t msg[8] = {
        0x01, // slave address
        0x04, // reads_regs
        0, 0, // start_reg
        0, 10, // num regs
    };
    const uint16_t crc16 = crc(msg, 6);
    msg[6] = uint8_t(crc16 & 0xff);
    msg[7] = uint8_t(crc16 >> 8);
    return out->tx((const char *) msg, sizeof(msg)) == sizeof(msg);
}

static uint16_t get2(const uint8_t *data, int *idx)
{
    uint16_t r = uint16_t((data[0] << 8) + data[1]);
    *idx += 2;
    return r;
}

static uint32_t get4(const uint8_t *data, int *idx)
{
    uint32_t r = get2(data, idx);
    r += 10000 * get2(& data[2], idx);
    return r;
}

bool PZEM004T::parse(struct PZEM004T::Status *status, const uint8_t *data, int n)
{
    ASSERT(data);
    ASSERT(n > 2); // must have more than just a crc
    // check CRC
    const uint16_t crc16 = crc(data, n-2);
    if (data[n-2] != (crc16 & 0xff))
    {
        return false;
    }
    if (data[n-1] != (crc16 >> 8))
    {
        return false;
    }

    if ((data[0]) != 0x01)
    {
        // slave id
        return false;
    }
    if ((data[1]) != 0x04)
    {
        // read regs request
        return false;
    }
    if ((data[2]) != 20)
    {
        // 10 regs == 20 bytes
        return false;
    }

    // if we aren't asked to convert the data, return true
    if (!status)
    {
        return true;
    }

    int idx = 3; // skip header
    status->volts           = float(get2(& data[idx], & idx) * 0.1);
    status->current         = float(get4(& data[idx], & idx) * 0.001);
    status->power           = float(get4(& data[idx], & idx) * 0.01);
    status->energy          = float(get4(& data[idx], & idx) * 1.0);
    status->freq            = float(get2(& data[idx], & idx) * 0.1);
    status->power_factor    = float(get2(& data[idx], & idx) * 0.01);

    return true;
}

uint16_t PZEM004T::crc(const uint8_t *data, int n)
{
    uint16_t crc = 0xffff;
    for (int i = 0; i < n; i++)
    {
        crc = uint16_t(crc ^ data[i]);
        for (int j = 0; j < 8; j++)
        {
            if (crc & 0x0001)
            {
                crc = uint16_t(crc >> 1);
                crc ^= 0xa001;
            }
            else
            {
                crc = uint16_t(crc >> 1);
            }
        }
    }
    return crc;
}

}   //  namespace panglos

//  FIN
