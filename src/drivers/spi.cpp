
#include "panglos/debug.h"

#include "panglos/mutex.h"
#include "panglos/drivers/gpio.h"

#include "panglos/drivers/spi.h"

namespace panglos {

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
    Lock lock(spi->mutex);

    cs->set(false);
    bool okay = spi->write(data, size);
    cs->set(true);
    return okay;
}

bool SpiDevice::read(const uint8_t *data, uint8_t *rd, int size)
{
    Lock lock(spi->mutex);

    cs->set(false);
    bool okay = spi->read(data, rd, size);
    cs->set(true);
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

    Lock lock(spi->mutex);

    cs->set(false);
    bool okay = spi->read(wr, rd, sizeof wr);
    cs->set(true);

    *data = rd[2];
    return okay;
}

}   //  namespace panglos

//  FIN
