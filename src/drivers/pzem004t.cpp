
#include <stdint.h>

#include "panglos/debug.h"
#include "panglos/io.h"

#include "panglos/drivers/pzem004t.h"

    /*
     *  Driver Code for the PZEM004T main monitor
     *
     *  Many thanks to TheHWcave for his work on this device
     *  which allowed me to make sense of the serial protocol.
     *
     *  https://www.youtube.com/watch?v=qRsjsenvlJA
     *  https://github.com/TheHWcave/Peacefair-PZEM-004T-
     */

namespace panglos {

    /*
     *  Helper functions to read 16 & 32-bit register data
     */
    
static uint16_t get16(const uint8_t *data, int *idx)
{
    // big-endian 16-bit data
    uint16_t r = uint16_t((data[0] << 8) + data[1]);
    *idx += 2;
    return r;
}

static float get32(const uint8_t *data, int *idx)
{
    uint32_t r = get16(data, idx);
    r += 10000 * get16(& data[2], idx);
    return float(r);
}

    /*
     *
     */

PZEM004T::PZEM004T(uint8_t _addr)
:   addr(_addr)
{
}

bool PZEM004T::request(Out *out)
{
    ASSERT(out);

    uint8_t msg[8] = {
        addr, // slave address
        READ_REGS, // reads_regs
        // big endian 16-bit fields :
        0, 0, // start_reg 
        0, 10, // num regs
    };
    const uint16_t crc16 = crc(msg, 6);
    msg[6] = uint8_t(crc16 & 0xff);
    msg[7] = uint8_t(crc16 >> 8);
    return out->tx((const char *) msg, sizeof(msg)) == sizeof(msg);
}

bool PZEM004T::crc_check(const uint8_t *data, int n)
{
    if (n <= 5)
    {
        // must have at least header + n + crc
        return false;
    }

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

    return true;
}

enum PZEM004T::Cmd PZEM004T::get_command(const uint8_t *data, int n)
{
    if (!crc_check(data, n))
    {
        return ERROR;
    }

    if ((data[0]) != addr)
    {
        return ERROR;
    }

    return Cmd(data[1]);
}

bool PZEM004T::parse(struct PZEM004T::Status *status, const uint8_t *data, int n)
{
    ASSERT(data);

    enum Cmd cmd = get_command(data, n);
    if (cmd != READ_REGS)
    {
        return false;
    }

    int idx = 2; // skip ADDR, CMD

    if ((data[idx++]) != 20)
    {
        // 10 regs == 20 bytes
        return false;
    }

    // if we aren't asked to convert the data, return true
    if (!status)
    {
        return true;
    }

    // data is returned as 16 or 32-bit fixed point
    status->volts        = get16(& data[idx], & idx) * 0.1F;
    status->current      = get32(& data[idx], & idx) * 0.001F;
    status->power        = get32(& data[idx], & idx) * 0.1F;
    status->energy       = get32(& data[idx], & idx) * 1.0F;
    status->freq         = get16(& data[idx], & idx) * 0.1F;
    status->power_factor = get16(& data[idx], & idx) * 0.01F;

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
