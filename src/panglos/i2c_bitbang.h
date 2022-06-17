
    /*
     *  Bitbang I2C
     */

#include "panglos/i2c.h"

namespace panglos {

class BitBang_I2C : public I2C
{
    Mutex *mutex;
    GPIO *sda;
    GPIO *scl;
    void (*wait_fn)();

    void wait();
    void start();
    bool bit_io(bool bit);
    bool ack();
    void stop();
    uint8_t io(uint8_t wr, bool *_ack);
    bool _io_write_read(uint8_t cmd, const uint8_t *wr, int wr_len, uint8_t *rd, int rd_len);
    bool io_write_read(uint8_t cmd, const uint8_t *wr, int wr_len, uint8_t *rd, int rd_len);

    uint8_t make_cmd(uint8_t addr, bool wr);

public:
    BitBang_I2C(Mutex *mutex, GPIO *_scl, GPIO *_sda, void (*_wait)());

    virtual bool probe(uint8_t addr, uint32_t timeout) override;
    virtual int write(uint8_t addr, const uint8_t* wr, uint32_t len) override;
    virtual int write_read(uint8_t addr, const uint8_t* wr, uint32_t len_wr, uint8_t* rd, uint32_t len_rd) override;
    virtual int read(uint8_t addr, uint8_t* rd, uint32_t len) override;
};

}   //  namespace panglos

//  FIN
