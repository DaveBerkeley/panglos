
#if !defined(__PANGLOS_SPI__)
#define __PANGLOS_SPI__

#include <stdint.h>

namespace panglos {

class Mutex;
class GPIO;

class SPI
{
public:
    Mutex *mutex;

    SPI(Mutex *m) : mutex(m) { }
    virtual ~SPI() { }

    virtual bool write(const uint8_t *data, int size) = 0;
    virtual bool io(const uint8_t *data, uint8_t *rd, int size) = 0;
};

    /*
     *
     */

class SpiDevice
{
    SPI *spi;
    GPIO *cs;
    uint8_t addr;

public:
    SpiDevice(SPI *_spi, GPIO *_cs, uint8_t _addr);

    // write a block of data
    bool write(const uint8_t *data, int size);
    bool io(const uint8_t *data, uint8_t *rd, int size);

    // register read / write
    bool write(uint8_t reg, uint8_t data);
    bool read(uint8_t reg, uint8_t *data);
};

}   //  namespace panglos

#endif // __PANGLOS_SPI__

//  FIN
