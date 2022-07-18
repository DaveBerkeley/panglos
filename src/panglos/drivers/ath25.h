
#if !defined(__PANGLOS_ATH25__)
#define __PANGLOS_ATH25__

#include "panglos/time.h"

namespace panglos {

class I2C;

class ATH25
{
    I2C *i2c;
public:
    static const uint8_t ADDR;

    enum Command {
        CMD_INIT = 0xe1,
        CMD_REQ =  0xac,
        CMD_RESET = 0xba,
    };

    ATH25(I2C* i2c);

    static bool probe(I2C* i2c);

    typedef struct {
        double temperature;
        double humidity;
    }   Reading;

    void init();
    Time::tick_t request();
    bool get(Reading *r);
};

}   //  namespace panglos

#endif  //  __PANGLOS_ATH25__

//  FIN
