
#if !defined(__PANGLOS_SPI_BITBANG_H__)
#define __PANGLOS_SPI_BITBANG_H__

#include "panglos/drivers/spi.h"

namespace panglos {

class GPIO;
class Mutex;

class SPI_BitBang : public SPI
{
    SPI::Lock cs_lock;
    GPIO *copi;
    GPIO *cipo;
    GPIO *ck;
    bool cpol;
    bool cpha;

    uint8_t write(uint8_t data);

public:
    SPI_BitBang(Mutex *mutex, GPIO *copi, GPIO *cipo, GPIO *ck, bool cpol=false, bool cpha=false);

    virtual bool read(uint8_t *data, int size) override;

    virtual bool write(const uint8_t *data, int size) override;
    virtual bool io(const uint8_t *data, uint8_t *rd, int size) override;

    virtual uint32_t set_speed(uint32_t hz) override;
    virtual void set_blocking(bool blocking) override;
    virtual Lock *lock() override { return & cs_lock; };
    
};

}   //  namespace panglos

#endif  //  __PANGLOS_SPI_BITBANG_H__

//  FIN
