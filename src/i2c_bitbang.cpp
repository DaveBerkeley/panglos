
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
        wait_fn(wait_arg);
    }
}

void BitBang_I2C::restart()
{
    if (verbose) PO_DEBUG("");

    sda->set(1);
    wait();
    scl->set(1);
    wait();
    sda->set(0);
    wait();
    scl->set(0);
    wait();
}

void BitBang_I2C::start()
{
    if (verbose) PO_DEBUG("");

    scl->set(1);
    wait();
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
    sda->set(1);
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
    if (verbose) PO_DEBUG("wr=%#x", wr);
    uint8_t rd = 0;
    for (int i = 0; i < 8; i++)
    {
        bool d = bit_io(!!(wr & 0x80));
        wr = (uint8_t) (wr << 1);
        rd = (uint8_t) (rd << 1);
        if (d) rd |= 1;
    }
    bool a = ack();
    if (verbose) PO_DEBUG("got %s", a ? "ACK" : "NAK");
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

    ASSERT(!rd_len);
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

#define WR(addr) ((uint8_t) ((addr << 1)))
#define RD(addr) ((uint8_t) ((addr << 1) + 1))

    /*
     *
     */

BitBang_I2C::BitBang_I2C(Mutex *m, GPIO *_scl, GPIO *_sda, void (*_wait)(void *), void *arg, bool _verbose)
:   I2C(m),
    sda(_sda),
    scl(_scl),
    wait_fn(_wait),
    wait_arg(arg),
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

    start();
    bool ok = _read(addr, 0, 0);
    stop();
    return ok;
}

int BitBang_I2C::write(uint8_t addr, const uint8_t* wr, uint32_t len)
{
    Lock lock(mutex);

    const bool ok = io_write_read(WR(addr), wr, len, 0, 0);
    return ok ? len : 0;
}

bool BitBang_I2C::_write_read(uint8_t addr, const uint8_t* wr, uint32_t len_wr, uint8_t* rd, uint32_t len_rd)
{
    bool ack = false;

    io(WR(addr), & ack);
    if (!ack)
    {
        return false;
    }

    ASSERT(len_wr == 1);
    ASSERT(wr);

    io(*wr, & ack);
    if (!ack)
    {
        return false;
    }

    restart();

    io(RD(addr), & ack);
    if (!ack)
    {
        return false;
    }

    int n = 0;

    for (uint32_t i = 0; i < len_rd; i++)
    {
        *rd++ = io(0xff, & ack);
        if (!ack)
        {
            PO_ERROR("i=%d", i);
            return false;
        }
        n += 1;
    }

    return n;
}

int BitBang_I2C::write_read(uint8_t addr, const uint8_t* wr, uint32_t len_wr, uint8_t* rd, uint32_t len_rd)
{
    ASSERT(len_wr && len_rd);
    PO_ERROR("addr=%#x wr=%#x", addr, *wr);

    Lock lock(mutex);

    start();
    bool ok = _write_read(addr, wr, len_wr, rd, len_rd);
    stop();
    return ok ? len_rd : 0;
}

bool BitBang_I2C::_read(uint8_t addr, uint8_t* rd, uint32_t len)
{
    //PO_DEBUG("addr=%#x", addr);
    bool ack = false;

    io(RD(addr), & ack);
    if (!ack)
    {
        PO_ERROR("nak");
        return false;
    }

    for (uint32_t i = 0; i < len; i++)
    {
        *rd++ = io(0xff, & ack);
        if (!ack)
        {
            PO_ERROR("nak");
            return false;
        }
    }

    return true;
}

int BitBang_I2C::read(uint8_t addr, uint8_t* rd, uint32_t len)
{
    Lock lock(mutex);

    start();
    bool ok = _read(addr, rd, len);
    stop();
    return ok ? len : 0;
}

}   //  namespace panglos

//  FIN
