
#if !defined(__PANGLOS_AHT25__)
#define __PANGLOS_AHT25__

#include "panglos/time.h"

namespace panglos {

class I2C;

class AHT25
{
    I2C *i2c;
public:
    static const uint8_t ADDR;

    enum Command {
        CMD_INIT = 0xe1,
        CMD_REQ =  0xac,
        CMD_RESET = 0xba,
        CMD_STATUS = 0x71,
    };

    AHT25(I2C* i2c);

    static bool probe(I2C* i2c);

    typedef struct {
        double temperature;
        double humidity;
    }   Reading;

    bool init();
    int request(); // returns ms to wait
    bool get(Reading *r);
    uint8_t status();

    bool busy();
    bool calibrated();
};

}   //  namespace panglos

#endif  //  __PANGLOS_AHT25__

//  FIN
