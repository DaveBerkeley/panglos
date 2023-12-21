
#if !defined(__PANGLOS_7_SEGMENT__)
#define __PANGLOS_7_SEGMENT__

    /*
     *
     */

namespace panglos {

class I2C;

class Display
{
public:
    virtual ~Display(){}
    virtual void write(const char *text) = 0;
    virtual void set_brightness(uint8_t level) {}
};

class SevenSegment : public Display
{
    panglos::I2C *i2c;
    uint8_t addr;
    bool colon;

    static const char *next_seg(const char *text, uint8_t *seg);
public:
    SevenSegment(panglos::I2C *_i2c, uint8_t _addr=0x70);
    SevenSegment();

    bool probe();

    static uint8_t seg(char data, bool reverse=false);

    void init();

    void set_colon(bool state);

    virtual void write(const char *text) override;
};

}   //  namespace panglos

#endif  //  __PANGLOS_7_SEGMENT__

//  FIN
