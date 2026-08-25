
    /*
     *  Bitbang I2C
     */

#include <stdint.h>

#include "panglos/debug.h"
#include "panglos/mutex.h"
#include "panglos/drivers/gpio.h"

#include "panglos/drivers/i2c_bitbang.h"

namespace panglos {

#define WR(addr) ((uint8_t) ((addr << 1)))
#define RD(addr) ((uint8_t) ((addr << 1) + 1))

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

bool BitBang_I2C::ack()
{
    if (verbose) PO_DEBUG("");

    sda->set(1);
    wait();
    scl->set(1);
    const bool d = sda->get();
    if (verbose) PO_DEBUG("ack=%d", d);
    wait();
    scl->set(0);
    wait();

    return !d;
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

uint8_t BitBang_I2C::io(uint8_t wr, bool *nak, bool write)
{
    // always starts with clock lo
    //ASSERT(!scl->get());

    // use 16-bit data as we have 8-data data bits plus the ack/nak
    uint16_t data = 0;
    uint16_t send = (wr << 1) + (write ? 0x01 : 0);

    for (int i = 0; i < 9; i++)
    {
        bool tx = send & 0x100;
        send <<= 1;
        sda->set(tx);
        wait();
        scl->set(1);
        // TODO : clock stretching here? Wait for line to go hi
        bool rx = sda->get();
        wait();
        scl->set(0);
        wait();
        data <<= 1;
        data |= rx ? 0x01 : 0;
    }

    // ACK/NAK
    ASSERT(nak);
    *nak = data & 0x01;

    return uint8_t(data >> 1);
}

bool BitBang_I2C::_io_write_read(uint8_t addr, const uint8_t *wr, int wr_len, uint8_t *rd, int rd_len)
{
    bool nak;

    // send initial command byte
    io(WR(addr), & nak, true);
    if (nak) return false;

    // do write
    for (int i = 0; i < wr_len; i++)
    {
        io(*wr++, & nak, true);
        if (nak) return false;
    }

    if (!rd) return true;

    restart();

    // send read command byte
    io(RD(addr), & nak, true);
    if (nak) return false;

    // do read
    for (int i = 0; i < rd_len; i++)
    {
        const bool last = i == (rd_len - 1);
        *rd++ = io(0xff, & nak, last);
        if (nak != last) return false;
    }

    return true;
}

bool BitBang_I2C::io_write_read(uint8_t addr, const uint8_t *wr, int wr_len, uint8_t *rd, int rd_len)
{
    start();
    bool ok = _io_write_read(addr, wr, wr_len, rd, rd_len);
    stop();
    if (verbose && !ok) PO_DEBUG("NAK");
    return ok;
}

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
    if (verbose) PO_DEBUG("addr=%#x", addr);
    IGNORE(timeout);

    Lock lock(mutex);
    return io_write_read(addr, 0, 0, 0, 0);
}

int BitBang_I2C::write(uint8_t addr, const uint8_t* wr, uint32_t len)
{
    Lock lock(mutex);

    const bool ok = io_write_read(addr, wr, int(len), 0, 0);
    return ok ? int(len) : 0;
}

int BitBang_I2C::write_read(uint8_t addr, const uint8_t* wr, uint32_t len_wr, uint8_t* rd, uint32_t len_rd)
{
    ASSERT(len_wr && len_rd);
    if (verbose) PO_ERROR("addr=%#x wr=%#x", addr, *wr);

    Lock lock(mutex);

    const bool ok = io_write_read(addr, wr, len_wr, rd, len_rd);
    return ok ? int(len_rd) : 0;
}

int BitBang_I2C::read(uint8_t addr, uint8_t* rd, uint32_t len)
{
    Lock lock(mutex);

    const bool ok = io_write_read(addr, 0, 0, rd, len);
    return ok ? int(len) : 0;
}

}   //  namespace panglos

//  FIN
