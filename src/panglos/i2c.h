
namespace panglos {

class I2C
{
public:
    virtual ~I2C() {}

    virtual bool probe(uint8_t addr, uint32_t timeout) = 0;
    virtual int write(uint8_t addr, const uint8_t* wr, uint32_t len) = 0;
    virtual int write_read(uint8_t addr, const uint8_t* wr, uint32_t len_wr, uint8_t* rd, uint32_t len_rd) = 0;
    virtual int read(uint8_t addr, uint8_t* rd, uint32_t len) = 0;

    static I2C *create(int idx, Mutex *m);
};

}   //  panglos

//  FIN
