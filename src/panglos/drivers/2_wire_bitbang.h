
#include "panglos/drivers/i2c.h"

namespace panglos {

class TwoWire : public I2C
{
    GPIO *scl;
    GPIO *sda;

    void start();
    void stop();
    bool bit(bool tx);
    bool tx(uint8_t data, uint8_t *rx);
    void wait();

    int io(const uint8_t* wr, uint32_t len_wr, uint8_t* rd, uint32_t len_rd);
public:
    TwoWire(GPIO *scl, GPIO *sda, Mutex *);

    virtual bool probe(uint8_t addr, uint32_t timeout) override;
    virtual int write(uint8_t addr, const uint8_t* wr, uint32_t len) override;
    virtual int write_read(uint8_t addr, const uint8_t* wr, uint32_t len_wr, uint8_t* rd, uint32_t len_rd) override;
    virtual int read(uint8_t addr, uint8_t* rd, uint32_t len) override;
};

}   //  namesapce panglos

//  FIN
