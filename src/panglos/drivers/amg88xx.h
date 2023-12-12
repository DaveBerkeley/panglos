
namespace panglos {

class I2C;

class AMG88xx
{
    I2C *i2c;
    uint8_t addr;

public:

    enum Regs {
        PowerControl = 0,
        Reset = 1,
        FrameRate = 2,
        InterruptControl = 3,
        Status = 4,
        StatusClear = 5,
        Pixel = 0x80,
    };

    AMG88xx(I2C *_i2c);
    bool read_frame(uint8_t *data, size_t size);
    bool probe();
    void init();
};

}   //  namespace panglos

//  FIN
