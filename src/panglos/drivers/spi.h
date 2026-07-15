
#if !defined(__PANGLOS_SPI__)
#define __PANGLOS_SPI__

#include <stdint.h>

namespace panglos {

class Mutex;
class GPIO;

class SPI
{
public:
    virtual ~SPI() { }

    virtual bool read(uint8_t *data, int size) = 0;
    virtual bool write(const uint8_t *data, int size) = 0;
    virtual bool io(const uint8_t *data, uint8_t *rd, int size) = 0;

    virtual void set_blocking(bool blocking) = 0;
    virtual uint32_t set_speed(uint32_t hz) = 0;

    /*
     *
     */

    class Lock
    {
        Mutex *mutex;
    public:
        Lock(Mutex *m) : mutex(m) { }

        void lock(GPIO *cs);
        void unlock(GPIO *cs);
    };

    virtual Lock *lock() = 0;

    class CS_Lock
    {
        Lock *lock;
        GPIO *cs;
    public:
        CS_Lock(SPI *spi, GPIO *cs);
        ~CS_Lock();
    };
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
    SpiDevice(SPI *spi, GPIO *cs, uint8_t addr);

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
