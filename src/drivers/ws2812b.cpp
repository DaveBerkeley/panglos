
    /*
     *
     */

#include <stdint.h>
#include <math.h>
#include <sys/time.h>

#include "panglos/debug.h"

#include "panglos/drivers/ws2812b.h"

namespace panglos {

void LedCircle::calc_angle(int _360, struct Hand *hand, uint8_t _max)
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

void LedCircle::set_angle(enum LedStrip::Colour colour, const struct Hand *hand)
{
    ASSERT(hand);
    switch (colour)
    {
        case LedStrip::R :
        {
            rgb[hand->idx0].r = hand->bright0;
            rgb[hand->idx1].r = hand->bright1;
            break;
        }
        case LedStrip::G :
        {
            rgb[hand->idx0].g = hand->bright0;
            rgb[hand->idx1].g = hand->bright1;
            break;
        }
        case LedStrip::B :
        {
            rgb[hand->idx0].b = hand->bright0;
            rgb[hand->idx1].b = hand->bright1;
            break;
        }
        default : ASSERT(0);
    }
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

    const double fsecs = tm.tm_sec + (tv.tv_usec / 1000000.0);
    const double fmins = tm.tm_min + (fsecs / 60.0);
    const double fhours = fmod(tm.tm_hour + (fmins / 60.0), 12.0);

    struct Hand hand;

    calc_angle(fsecs * 6, & hand, 0x40);
    set_angle(LedStrip::B, & hand);

    calc_angle(fmins * 6, & hand, 0x80);
    set_angle(LedStrip::R, & hand);

    calc_angle(fhours * 30, & hand, 0xff);
    set_angle(LedStrip::G, & hand);
    return true;
}

void LedCircle::draw()
{
    for (int i = 0; i < leds->num_leds(); i++)
    {
        struct LedStrip::RGB *x = & rgb[i];
        leds->set(i, x->r, x->g, x->b);
    }
    leds->send();
}

void LedCircle::set_all(uint8_t r, uint8_t g, uint8_t b)
{
    for (int i = 0; i < leds->num_leds(); i++)
    {
        struct LedStrip::RGB *x = & rgb[i];
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
    rgb = new struct LedStrip::RGB[leds->num_leds()];
}

LedCircle::~LedCircle()
{
    delete[] rgb;
}

    /*
     *
     */

enum Scale::Section Scale::zone(double value)
{
    if (value < lo) return LO;
    if (value < (lo + (1 * step))) return _B;
    if (value < (lo + (2 * step))) return BG;
    if (value < (lo + (3 * step))) return GB;
    if (value < (lo + (4 * step))) return GR;
    if (value < (lo + (5 * step))) return RG;
    return HI;
}

void Scale::set(LedStrip::RGB *rgb, uint8_t r, uint8_t g, uint8_t b)
{
    ASSERT(rgb);
    rgb->r = r;
    rgb->g = g;
    rgb->b = b;
}

uint8_t Scale::ramp(double part)
{
    return 255 * part;
}

uint8_t Scale::reduce(double part)
{
    return 255 - ramp(part);
}

void Scale::scale(LedStrip::RGB *rgb, double value)
{
    const double part = fmod(value - lo, step) / step;
    //PO_DEBUG("%f %f", value, part);
    switch (zone(value))
    {
        case LO : set(rgb, 0, 0, 0); break;
        case _B : set(rgb, 0, 0, ramp(part)); break;
        case BG : set(rgb, 0, ramp(part), 0xff); break;
        case GB : set(rgb, 0, 0xff, reduce(part)); break;
        case GR : set(rgb, ramp(part), 0xff, 0); break;
        case RG : set(rgb, 0xff, reduce(part), 0); break;
        case HI : set(rgb, 0xff, 0xff, 0xff); break;
    }
}

Scale::Scale(double _lo, double _hi)
:   lo(_lo),
    hi(_hi),
    step((hi - lo) / 5)
{
}

}   //  namespace panglos

//  FIN
