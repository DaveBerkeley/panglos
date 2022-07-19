
#include <stdint.h>

#include "panglos/debug.h"
#include "panglos/time.h"

#include "panglos/drivers/gpio.h"
#include "panglos/drivers/pwm.h"

namespace panglos {

    /*
     *  SK68xx driver
     */

static void wait(int n)
{
    // Wait function for bitbanged interface
    for (volatile int i = 0; i < n; i++)
    {
    }
}

static void write_bit(panglos::GPIO *gpio, bool state)
{
    gpio->set(1);
    //wait(1);
    if (!state) gpio->set(0);
    wait(8);
    gpio->set(0);
    //wait(1);
}

PWM_SK68xx::PWM_SK68xx(panglos::GPIO *_gpio)
:   gpio(_gpio)
{
    ASSERT(gpio);
    rgb[0] = rgb[1] = rgb[2] = 0;
    set(0, 0, false);
    set(1, 0, false);
    set(2, 0, false);
    flush();
}

bool PWM_SK68xx::set(int idx, uint32_t value, bool _flush)
{
    if ((idx < 0) || (idx > 2)) return false;

    rgb[2 - idx] = value >> 6;

    if (_flush) flush();

    return true;
}

void PWM_SK68xx::flush()
{
    gpio->set(0);

    for (int byte = 0; byte < 3; byte++)
    {
        uint8_t data = rgb[byte];
        for (int bit = 0; bit < 8; bit++)
        {
            write_bit(gpio, data & 0x80);
            data <<= 1;
        }
    }

    // reset
    gpio->set(0);
    Time::msleep(2);
}

}   //  namespace panglos

//  FIN

