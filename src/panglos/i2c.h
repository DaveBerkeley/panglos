
#if !defined(__PANGLOS_I2C__)
#define __PANGLOS_I2C__

namespace panglos {

class I2C
{
public:
    typedef enum {
        ID_0,
        ID_1
    }   ID;

    virtual ~I2C() {}

    virtual bool probe(uint8_t addr, uint32_t timeout) = 0;
    virtual int write(uint8_t addr, const uint8_t* wr, uint32_t len) = 0;
    virtual int write_read(uint8_t addr, const uint8_t* wr, uint32_t len_wr, uint8_t* rd, uint32_t len_rd) = 0;
    virtual int read(uint8_t addr, uint8_t* rd, uint32_t len) = 0;

    static I2C *create(ID idx, Mutex *m);
};

}   //  panglos

#endif  //  __PANGLOS_I2C__

//  FIN
