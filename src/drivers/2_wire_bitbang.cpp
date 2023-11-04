
#include <stdint.h>

#include "panglos/debug.h"

#include "panglos/mutex.h"
#include "panglos/drivers/gpio.h"

#include "panglos/drivers/2_wire_bitbang.h"

namespace panglos {

    /*
     *
     */

TwoWire::TwoWire(GPIO *_scl, GPIO *_sda, Mutex *m)
:   I2C(m),
    scl(_scl),
    sda(_sda)
{
    ASSERT(sda);
    ASSERT(scl);
    sda->set(1);
    scl->set(1);
}

void TwoWire::wait(int n)
{
    // TODO
    for (int j = 0; j < n; j++)
    {
        for (volatile int i = 0; i < 100; i++)
        {
        }
    }
}

void TwoWire::start()
{
    // pull data low while clock hi
    wait();
    scl->set(1);
    wait();
    sda->set(0);
    wait();
}

void TwoWire::stop()
{
    // pull data hi while clock hi
    sda->set(0);
    wait();
    scl->set(1);
    wait();
    sda->set(1);
    wait();
}

bool TwoWire::bit(bool tx)
{
    scl->set(0);
    wait();
    sda->set(tx);
    wait();
    scl->set(1);
    const bool rx = sda->get();
    wait();
    scl->set(0);
    wait();
    return rx;
}    

bool TwoWire::tx(uint8_t data, uint8_t *rx)
{
    uint8_t rd = 0;
    // send 8 bits
    for (int i = 0; i < 8; i++)
    {
        const bool r = bit(data & 0x01);
        data >>= 1;
        rd >>= 1;
        if (r)
        {
            rd |= 0x80;
        }
    }
    //  get the ACK bit : device will pull the sda line low
    const bool ack = bit(1);
    if (rx)
    {
        *rx = rd;
    }

    wait(5);

    return !ack;
}

bool TwoWire::probe(uint8_t addr, uint32_t timeout) 
{
    IGNORE(addr);
    IGNORE(timeout);
    ASSERT(0);
    return false;
}

int TwoWire::io(const uint8_t* wr, uint32_t len_wr, uint8_t* rd, uint32_t len_rd)
{
    for (uint32_t i = 0; i < len_wr; i++)
    {
        if (!tx(wr[i], 0))
        {
            return 0;
        }
    }

    uint8_t hi = 0xff;

    for (uint32_t i = 0; i < len_rd; i++)
    {
        if (!tx(hi, & rd[i]))
        {
            return 0;
        }
    }

    return len_wr + len_rd;
}

    /*
     *
     */

int TwoWire::write_read(uint8_t addr, const uint8_t* wr, uint32_t len_wr, uint8_t* rd, uint32_t len_rd)
{
    IGNORE(addr);
    start();
    const int n = io(wr, len_wr, rd, len_rd);
    stop();
    wait(20);
    return n;
}

int TwoWire::write(uint8_t addr, const uint8_t* wr, uint32_t len)
{
    return write_read(addr, wr, len, 0, 0);
}

int TwoWire::read(uint8_t addr, uint8_t* rd, uint32_t len)
{
    return write_read(addr, 0, 0, rd, len);
}

}   //  namespace panglos

//  FIN
