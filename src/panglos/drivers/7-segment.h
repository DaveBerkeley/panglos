
#if !defined(__PANGLOS_7_SEGMENT__)
#define __PANGLOS_7_SEGMENT__

    /*
     *
     */

namespace panglos {

class I2C;

class SevenSegment
{
    panglos::I2C *i2c;
    uint8_t addr;
    bool colon;

    static const char *next_seg(const char *text, uint8_t *seg);
public:
    SevenSegment(panglos::I2C *_i2c, uint8_t _addr=0x70);

    bool probe();

    static uint8_t seg(char data);

    void init();

    void set_colon(bool state);

    void write(const char *text);
};

}   //  namespace panglos

#endif  //  __PANGLOS_7_SEGMENT__

//  FIN
