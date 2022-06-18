
#if !defined(__PANGLOS_SPI__)
#define __PANGLOS_SPI__

#include <stdint.h>

#include "gpio.h"
#include "mutex.h"

namespace panglos {

class SpiDevice;

class SPI
{
public:
    Mutex *mutex;

    enum ID {
        SPI_1,
        SPI_2
    };

    SPI(Mutex *m) : mutex(m) { }
    virtual ~SPI() { }

    virtual bool write(const uint8_t *data, int size) = 0;
    virtual bool read(const uint8_t *data, uint8_t *rd, int size) = 0;

    static SPI *create(ID _id, Mutex *mutex);
};

class SpiDevice
{
    SPI *spi;
    GPIO *cs;
    uint8_t addr;

public:
    SpiDevice(SPI *_spi, GPIO *_cs, uint8_t _addr);

    // write a block of data
    bool write(const uint8_t *data, int size);
    bool read(const uint8_t *data, uint8_t *rd, int size);

    // register read / write
    bool write(uint8_t reg, uint8_t data);
    bool read(uint8_t reg, uint8_t *data);
};

}   //  namespace panglos

#endif // __PANGLOS_SPI__

//  FIN
