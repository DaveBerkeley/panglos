
#if !defined(__PANGLOS_I2C_BITBANG__)
#define __PANGLOS_I2C_BITBANG__

    /*
     *  Bitbang I2C
     */

#include "panglos/i2c.h"

namespace panglos {

class BitBang_I2C : public I2C
{
    GPIO *sda;
    GPIO *scl;
    void (*wait_fn)(void *);
    void *wait_arg;
    bool verbose;

    void wait();

public:
    void start();
    void restart();
    bool bit_io(bool bit);
    bool ack();
    void stop();
    uint8_t io(uint8_t wr, bool *_ack);

private:
    bool _io_write_read(uint8_t cmd, const uint8_t *wr, int wr_len, uint8_t *rd, int rd_len);
    bool io_write_read(uint8_t cmd, const uint8_t *wr, int wr_len, uint8_t *rd, int rd_len);

    bool _read(uint8_t addr, uint8_t* rd, uint32_t len);
    bool _write_read(uint8_t addr, const uint8_t* wr, uint32_t len_wr, uint8_t* rd, uint32_t len_rd);

public:
    BitBang_I2C(Mutex *mutex, GPIO *_scl, GPIO *_sda, void (*_wait)(void*), void *wait_arg=0, bool verbose=false);
    ~BitBang_I2C();

    virtual bool probe(uint8_t addr, uint32_t timeout) override;
    virtual int write(uint8_t addr, const uint8_t* wr, uint32_t len) override;
    virtual int write_read(uint8_t addr, const uint8_t* wr, uint32_t len_wr, uint8_t* rd, uint32_t len_rd) override;
    virtual int read(uint8_t addr, uint8_t* rd, uint32_t len) override;
};

}   //  namespace panglos

#endif  //  __PANGLOS_I2C_BITBANG__

//  FIN
