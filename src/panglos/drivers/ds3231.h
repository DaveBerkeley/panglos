
#include "panglos/drivers/rtc.h"

namespace panglos {

class I2C;

class DS3231 : public RTC
{
    I2C *i2c;

public:
    static const uint8_t ADDR;

    DS3231(I2C *i2c);

    virtual bool init() override;

    virtual bool set(const DateTime *dt) override;
    virtual bool get(DateTime *dt) override;

    // Only exposed for unit tests
    static uint8_t from_bcd(uint8_t data);
    static uint8_t to_bcd(uint8_t data);
};

}   //  namespace panglos

//  FIN
