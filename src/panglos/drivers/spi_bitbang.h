
#if !defined(__PANGLOS_SPI_BITBANG_H__)
#define __PANGLOS_SPI_BITBANG_H__

#include "panglos/drivers/spi.h"

namespace panglos {

class GPIO;

class SPI_BitBang : public SPI
{
    GPIO *copi;
    GPIO *cipo;
    GPIO *ck;

    uint8_t write(uint8_t data);

public:
    SPI_BitBang(Mutex *m, GPIO *copi, GPIO *cipo, GPIO *ck);

    virtual bool write(const uint8_t *data, int size) override;
    virtual bool read(const uint8_t *data, uint8_t *rd, int size) override;
};

}   //  namespace panglos

#endif  //  __PANGLOS_SPI_BITBANG_H__

//  FIN
