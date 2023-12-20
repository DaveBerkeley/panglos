
#pragma once

namespace panglos {

class LedStrip
{
public:
    virtual ~LedStrip() {}

    virtual void set(int led, uint8_t r, uint8_t g, uint8_t b) = 0;
    virtual bool send() = 0;
    virtual int num_leds() = 0;
};

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

    struct RGB {
        uint8_t r;
        uint8_t g;
        uint8_t b;
        uint8_t w;
    };
private:
    LedStrip *leds;
public:
    struct RGB *rgb;

    void calc_angle(int _360, struct Hand *hand, uint8_t _max=0xff);

    enum Colour { R, G, B };
    void set_angle(enum Colour colour, const struct Hand *hand);

    bool set_time();

    void draw();
    void set_all(uint8_t r, uint8_t g, uint8_t b);
    void clear();

    LedCircle(LedStrip *_leds);
    ~LedCircle();
};

}   //  namespace panglos

//  FIN
