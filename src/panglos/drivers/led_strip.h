
#pragma once

namespace panglos {

class LedStrip
{
public:
    struct RGB {
        uint8_t r;
        uint8_t g;
        uint8_t b;
        uint8_t w;
    };
    enum Colour { R, G, B };

    virtual ~LedStrip() {}

    virtual void set(int led, uint8_t r, uint8_t g, uint8_t b) = 0;
    virtual void set_all(uint8_t r, uint8_t g, uint8_t b) = 0;
    virtual bool send() = 0;
    virtual int num_leds() = 0;
};

    /*
     *
     */

class LedCircle
{
public:
    struct Hand
    {
        int idx0;
        uint8_t bright0;
        int idx1;
        uint8_t bright1;
    };

private:
    LedStrip *leds;
public:
    struct LedStrip::RGB *rgb;

    void calc_angle(int _360, struct Hand *hand, uint8_t _max=0xff);

    void set_angle(enum LedStrip::Colour colour, const struct Hand *hand);
    int num_leds() { return leds->num_leds(); }

    bool set_time();

    void draw();
    void set_all(uint8_t r, uint8_t g, uint8_t b);
    void set(int led, uint8_t r, uint8_t g, uint8_t b);
    void clear();

    LedCircle(LedStrip *_leds);
    ~LedCircle();
};

    /*
     *
     */

class Scale
{
    const double lo;
    const double hi;
    const double step;

    enum Section { LO, _BB, BG, GB, GR, RG, HI };

    enum Section zone(double value);

    static void set(LedStrip::RGB *rgb, uint8_t r, uint8_t g, uint8_t b);

    uint8_t ramp(double part);
    uint8_t reduce(double part);
 
public:
    void scale(LedStrip::RGB *rgb, double value);

    Scale(double _lo, double _hi);
};

}   //  namespace panglos

//  FIN
