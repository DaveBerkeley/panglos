
#include "panglos/debug.h"

#include "panglos/drivers/gpio.h"

#include "panglos/drivers/spi_bitbang.h"

namespace panglos {

SPI_BitBang::SPI_BitBang(Mutex *m, GPIO *_copi, GPIO *_cipo, GPIO *_ck)
:   SPI(m),
    copi(_copi),
    cipo(_cipo),
    ck(_ck)
{
    PO_DEBUG("");
}

uint8_t SPI_BitBang::write(uint8_t data)
{
    uint8_t read = 0;

    for (int i = 0; i < 8; i++)
    {
        const bool bit = !!(data & 0x80);
        copi->set(bit);
        ck->set(1);
        if (cipo)
        {
            read = uint8_t(read << 1);
            if (cipo->get())
            {
                read = uint8_t(read + 1);
            }
        }
        ck->set(0);

        data = uint8_t(data << 1);
    }
    return read;
}

bool SPI_BitBang::write(const uint8_t *data, int size)
{
    for (int i = 0; i < size; i++)
    {
        write(*data++);
    }
    copi->set(0);

    return size;
}

bool SPI_BitBang::read(const uint8_t *data, uint8_t *rd, int size)
{
    for (int i = 0; i < size; i++)
    {
        *rd++ = write(*data++);
    }
    copi->set(0);

    return size;
}


}   //  namespace panglos

//  FIN
