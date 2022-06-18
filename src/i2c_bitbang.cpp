
    /*
     *  Bitbang I2C
     */

#include <stdint.h>

#include "panglos/debug.h"
#include "panglos/mutex.h"
#include "panglos/gpio.h"

#include "panglos/i2c_bitbang.h"

namespace panglos {

void BitBang_I2C::wait()
{
    if (wait_fn)
    {
        wait_fn();
    }
}

void BitBang_I2C::start()
{
    if (verbose) PO_DEBUG("");
    scl->set(1);
    sda->set(0);
    wait();
    scl->set(0);
    wait();
}

bool BitBang_I2C::bit_io(bool bit)
{
    sda->set(bit);
    wait();
    scl->set(1);
    wait();
    const bool d = sda->get();
    if (verbose) PO_DEBUG("wr=%d rd=%d", bit, d);
    scl->set(0);
    wait();
    return d;
}

bool BitBang_I2C::ack()
{
    if (verbose) PO_DEBUG("");
    // Handle clock stretching?
    return !bit_io(true);
}

void BitBang_I2C::stop()
{
    if (verbose) PO_DEBUG("");
    sda->set(0);
    wait();
    scl->set(1);
    wait();
    sda->set(1);
    wait();
}

uint8_t BitBang_I2C::io(uint8_t wr, bool *_ack)
{
    uint8_t rd = 0;
    for (int i = 0; i < 8; i++)
    {
        bool d = bit_io(!!(wr & 0x80));
        wr = (uint8_t) (wr << 1);
        rd = (uint8_t) (rd << 1);
        if (d) rd |= 1;
    }
    bool a = ack();
    if (_ack) *_ack = a;
    return rd;
}

bool BitBang_I2C::_io_write_read(uint8_t cmd, const uint8_t *wr, int wr_len, uint8_t *rd, int rd_len)
{
    bool ack = false;

    io(cmd, & ack);
    if (!ack)
    {
        return false;
    }

    for (int i = 0; i < wr_len; i++)
    {
        io(*wr++, & ack);
        if (!ack)
        {
            return false;
        }
    }
    for (int i = 0; i < rd_len; i++)
    {
        *rd++ = io(0, & ack);
        if (!ack)
        {
            return false;
        }
    }
    return ack;
}

bool BitBang_I2C::io_write_read(uint8_t cmd, const uint8_t *wr, int wr_len, uint8_t *rd, int rd_len)
{
    start();
    bool ack = _io_write_read(cmd, wr, wr_len, rd, rd_len);
    stop();
    return ack;
}

uint8_t BitBang_I2C::make_cmd(uint8_t addr, bool wr)
{
    return (uint8_t) ((addr << 1) + (wr ?  0 : 1));
}

    /*
     *
     */

BitBang_I2C::BitBang_I2C(Mutex *m, GPIO *_scl, GPIO *_sda, void (*_wait)(), bool _verbose)
:   mutex(m),
    sda(_sda),
    scl(_scl),
    wait_fn(_wait),
    verbose(_verbose)
{
    ASSERT(sda);
    ASSERT(scl);
}

BitBang_I2C::~BitBang_I2C()
{
}

bool BitBang_I2C::probe(uint8_t addr, uint32_t timeout)
{
    PO_DEBUG("addr=%#x", addr);
    IGNORE(timeout);
    Lock lock(mutex);

    const bool ok = io_write_read(make_cmd(addr, false), 0, 0, 0, 0);
    //PO_DEBUG("addr=%#x ack=%d", addr, ok);
    return ok;
}

int BitBang_I2C::write(uint8_t addr, const uint8_t* wr, uint32_t len)
{
    Lock lock(mutex);

    const bool ok = io_write_read(make_cmd(addr, true), wr, len, 0, 0);
    return ok ? len : 0;
}

int BitBang_I2C::write_read(uint8_t addr, const uint8_t* wr, uint32_t len_wr, uint8_t* rd, uint32_t len_rd)
{
    Lock lock(mutex);

    const bool ok = io_write_read(make_cmd(addr, true), wr, len_wr, rd, len_rd);
    return ok ? len_rd : 0;
}

int BitBang_I2C::read(uint8_t addr, uint8_t* rd, uint32_t len)
{
    Lock lock(mutex);

    PO_ERROR("TODO");
    IGNORE(addr);
    IGNORE(rd);
    IGNORE(len);
    ASSERT(0);
    return 0;
}

}   //  namespace panglos

//  FIN
