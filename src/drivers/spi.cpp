
#include "panglos/debug.h"

#include "panglos/mutex.h"
#include "panglos/drivers/gpio.h"

#include "panglos/drivers/spi.h"

namespace panglos {

    /*
     *
     */

void SPI::Lock::lock(GPIO *cs)
{
    if (mutex) mutex->lock();
    if (cs) cs->set(0);
}

void SPI::Lock::unlock(GPIO *cs)
{
    if (cs) cs->set(1);
    if (mutex) mutex->unlock();
}

    /*
     *
     */

SPI::CS_Lock::CS_Lock(SPI *spi, GPIO *_cs)
:   lock(spi->lock()),
    cs(_cs)
{
    lock->lock(cs);
}

SPI::CS_Lock::~CS_Lock()
{
    lock->unlock(cs);
}

    /*
     *
     */

SpiDevice::SpiDevice(SPI *_spi, GPIO *_cs, uint8_t _addr)
: spi(_spi), cs(_cs), addr(_addr)
{
    ASSERT(spi);
    ASSERT(cs);
}

bool SpiDevice::write(const uint8_t *data, int size)
{
    SPI::CS_Lock lock(spi, cs);

    bool okay = spi->write(data, size);
    return okay;
}

bool SpiDevice::io(const uint8_t *data, uint8_t *rd, int size)
{
    SPI::CS_Lock lock(spi, cs);

    bool okay = spi->io(data, rd, size);
    return okay;
}

bool SpiDevice::write(uint8_t reg, uint8_t data)
{
    uint8_t wr[] = { addr, reg, data };

    return write(wr, sizeof wr);
}

bool SpiDevice::read(uint8_t reg, uint8_t *data)
{
    ASSERT(data);
    uint8_t wr[] = { (uint8_t) (addr | 0x01), reg, 0 };
    uint8_t rd[3];

    SPI::CS_Lock lock(spi, cs);

    bool okay = spi->io(wr, rd, sizeof wr);

    *data = rd[2];
    return okay;
}

}   //  namespace panglos

//  FIN
