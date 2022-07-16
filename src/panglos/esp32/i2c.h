
#include "panglos/drivers/i2c.h"

namespace panglos {

class ESP_I2C : public I2C
{
    int chan;
    uint32_t scl;
    uint32_t sda;
    bool verbose;

    virtual bool probe(uint8_t addr, uint32_t timeout) override;
    virtual int write(uint8_t addr, const uint8_t* wr, uint32_t len) override;
    virtual int write_read(uint8_t addr, const uint8_t* wr, uint32_t len_wr, uint8_t* rd, uint32_t len_rd) override;
    virtual int read(uint8_t addr, uint8_t* rd, uint32_t len) override;

public:
    ESP_I2C(int chan, uint32_t scl, uint32_t sda, Mutex *mutex, bool verbose=false);
    virtual ~ESP_I2C();
};

}   //  namespace panglos

//  FIN
