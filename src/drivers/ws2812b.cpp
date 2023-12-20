
    /*
     *
     */

#include <stdint.h>
#include <sys/time.h>

#include "panglos/debug.h"

#include "panglos/drivers/ws2812b.h"

namespace panglos {

void LedCircle::get_angle(int _360, struct Hand *hand, uint8_t _max)
{
    double fidx = _360 * leds->num_leds() / 360.0;
    int idx = int(fidx);
    double frac = fidx - idx;
    double craf = 1.0 - frac;

    hand->idx0 = idx;
    hand->bright0 = uint8_t(_max * craf);
    hand->idx1 = (idx+1) % leds->num_leds();
    hand->bright1 = uint8_t(_max * frac);
}

bool LedCircle::set_time()
{
    struct timeval tv;
    gettimeofday(& tv, NULL);
    struct tm tm;
    gmtime_r(& tv.tv_sec, & tm);

    const int year = 1900 + tm.tm_year;
    if (year < 2023)
    {
        return false;
    }

    struct Hand hand;

    get_angle(tm.tm_sec * 6, & hand, 0x40);
    rgb[hand.idx0].b = hand.bright0;
    rgb[hand.idx1].b = hand.bright1;

    get_angle(tm.tm_min * 6, & hand, 0x80);
    rgb[hand.idx0].r = hand.bright0;
    rgb[hand.idx1].r = hand.bright1;

    get_angle((tm.tm_hour * 30) % 360, & hand, 0xff);
    rgb[hand.idx0].g = hand.bright0;
    rgb[hand.idx1].g = hand.bright1;
    return true;
}

void LedCircle::draw()
{
    for (int i = 0; i < leds->num_leds(); i++)
    {
        struct RGB *x = & rgb[i];
        leds->set(i, x->r, x->g, x->b);
    }
    leds->send();
}

void LedCircle::set_all(uint8_t r, uint8_t g, uint8_t b)
{
    for (int i = 0; i < leds->num_leds(); i++)
    {
        struct RGB *x = & rgb[i];
        x->r = r;
        x->g = g;
        x->b = b;
        x->w = 0;
    }
}

void LedCircle::clear()
{
    set_all(0, 0, 0);
}

LedCircle::LedCircle(LedStrip *_leds)
:   leds(_leds),
    rgb(0)
{
    ASSERT(leds);
    rgb = new struct RGB[leds->num_leds()];
}

LedCircle::~LedCircle()
{
    delete[] rgb;
}

}   //  namespace panglos

//  FIN
