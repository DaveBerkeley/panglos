
#include "panglos/debug.h"

#include "panglos/drivers/gpio.h"

#include "panglos/drivers/spi_bitbang.h"

namespace panglos {

SPI_BitBang::SPI_BitBang(Mutex *mutex, GPIO *_copi, GPIO *_cipo, GPIO *_ck, bool _cpol, bool _cpha)
:   cs_lock(mutex), 
    copi(_copi),
    cipo(_cipo),
    ck(_ck),
    cpol(_cpol),
    cpha(_cpha)
{
    PO_DEBUG("");
    ck->set(cpol);
}

uint8_t SPI_BitBang::write(uint8_t data)
{
    uint8_t read = 0;

    for (int i = 0; i < 8; i++)
    {
        read <<= 1;
        const bool bit = !!(data & 0x80);
        copi->set(bit);
        ck->set(1 ^ cpol);
        if (cipo && !cpha)
        {
            read |= cipo->get() ? 0x01 : 0;
        }
        ck->set(0 ^ cpol);
        if (cipo && cpha)
        {
            read |= cipo->get() ? 0x01 : 0;
        }

        data = uint8_t(data << 1);
    }
    return read;
}

bool SPI_BitBang::io(const uint8_t *data, uint8_t *rd, int size)
{
    for (int i = 0; i < size; i++)
    {
        const uint8_t tx = data ? *data++ : 0xff;
        const uint8_t rx = write(tx);
        if (rd) *rd++ = rx;
    }
    copi->set(0);

    return size;
}

bool SPI_BitBang::write(const uint8_t *data, int size)
{
    return io(data, 0, size);
}

bool SPI_BitBang::read(uint8_t *data, int size)
{
    return io(0, data, size);
}

uint32_t SPI_BitBang::set_speed(uint32_t)
{
    ASSERT(0);
    return 0;
}

void SPI_BitBang::set_blocking(bool)
{
    ASSERT(0);
}

}   //  namespace panglos

//  FIN
