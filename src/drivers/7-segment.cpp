
#include <stdint.h>

#include "panglos/debug.h"

#include "panglos/mutex.h"
#include "panglos/drivers/i2c.h"
#include "panglos/drivers/7-segment.h"

namespace panglos {

    /*
     *
     */


SevenSegment::SevenSegment(panglos::I2C *_i2c, uint8_t _addr)
:   i2c(_i2c),
    addr(_addr),
    colon(false)
{
    ASSERT(i2c);
}

bool SevenSegment::probe()
{
    return i2c->probe(addr, 1);
}

uint8_t SevenSegment::seg(char data, bool reverse)
{
    // https://en.wikipedia.org/wiki/Seven-segment_display
    uint8_t a = 0x01;
    uint8_t b = 0x02;
    uint8_t c = 0x04;
    uint8_t d = 0x08;
    uint8_t e = 0x10;
    uint8_t f = 0x20;
    uint8_t g = 0x40;
    uint8_t dp = 0x80;

    if (reverse)
    {
        a = 0x40;
        b = 0x20;
        c = 0x10;
        d = 0x08;
        e = 0x04;
        f = 0x02;
        g = 0x01;
        dp = 0x80;
    }

    switch (data)
    {
        case 0x01 : return a;
        case 0x02 : return b;
        case 0x03 : return c;
        case 0x04 : return d;
        case 0x05 : return e;
        case 0x06 : return f;
        case 0x07 : return g;

        case '0' : return uint8_t(a+b+c+d+e+f);
        case '1' : return uint8_t(b+c);
        case '2' : return uint8_t(a+b+g+e+d);
        case '3' : return uint8_t(a+b+g+c+d);
        case '4' : return uint8_t(f+b+g+c);
        case '5' : return uint8_t(a+f+g+c+d);
        case '6' : return uint8_t(a+f+g+e+c+d);
        case '7' : return uint8_t(a+b+c);
        case '8' : return uint8_t(a+f+b+g+e+c+d);
        case '9' : return uint8_t(a+f+b+g+c+d);

        case ' ' : return 0;
        case '.' : return dp;
        case ',' : return dp;
        case '+' : return uint8_t(b+g+c);
        case '-' : return g;
        case '=' : return g+d;
        case '"' : return f+b;
        case '\'': return b;
        case '!' : return uint8_t(b+c+dp);
        case '|' : return f+e;
        case '(' :
        case '{' :
        case '[' : return uint8_t(a+f+e+d);
        case ')' :
        case '}' :
        case ']' : return uint8_t(a+b+c+d);
        case '?' : return uint8_t(a+b+g+e);
        case '*' : return uint8_t(f+b+g+e+c);
        case '#' : return uint8_t(a+f+b+g);
        case '/' : return uint8_t(b+g+e);
        case '\\': return uint8_t(f+g+c);
        case ':' : return uint8_t(g+d);
        case ';' : return uint8_t(g+d);
        case '$' : return uint8_t(a+f+g+c+d);
        case '%' : return uint8_t(b+g+e);
        case '&' : return uint8_t(a+f+b+g+e);
        case '_' : return d;
        case '^' : return uint8_t(a+f+b);
        case '`' : return f;
        case '@' : return uint8_t(a+f+b+g+e+d);
        case '>' : return uint8_t(g+c+d);
        case '<' : return uint8_t(g+e+d);
        case '~' : return a;

        case 'a' :
        case 'A' : return uint8_t(a+f+b+g+e+c);
        case 'b' : return uint8_t(f+g+e+c+d);
        case 'B' : return uint8_t(f+g+e+c+d);
        case 'c' : return uint8_t(g+e+d);
        case 'C' : return uint8_t(a+f+e+d);
        case 'd' : return uint8_t(b+g+e+c+d);
        case 'D' : return uint8_t(a+f+b+e+c+d);
        case 'e' :
        case 'E' : return uint8_t(a+f+g+e+d);
        case 'f' :
        case 'F' : return uint8_t(a+f+g+e);
        case 'g' :
        case 'G' : return uint8_t(a+f+b+g+c+d);
        case 'h' : return uint8_t(f+g+e+c);
        case 'H' : return uint8_t(f+b+g+e+c);
        case 'i' : 
        case 'I' : return b+c;
        case 'j' :
        case 'J' : return uint8_t(b+c+d);
        case 'k' : return uint8_t(f+b+g+e+c);
        case 'K' : return uint8_t(f+b+g+e+c);
        case 'l' :
        case 'L' : return uint8_t(f+e+d);
        case 'm' : return uint8_t(g+e+c);
        case 'M' : return uint8_t(a+f+b+e+c);
        case 'n' : return uint8_t(g+e+c);
        case 'N' : return uint8_t(a+f+b+e+c);
        case 'o' : return uint8_t(g+e+c+d);
        case 'O' : return uint8_t(a+f+b+e+c+d);
        case 'p' :
        case 'P' : return uint8_t(a+f+b+g+e);
        case 'q' :
        case 'Q' : return uint8_t(a+f+b+g+c);
        case 'r' :
        case 'R' : return g+e;
        case 's' :
        case 'S' : return uint8_t(a+f+g+c+d);
        case 't' : 
        case 'T' : return uint8_t(f+g+e+d);
        case 'u' : return uint8_t(e+c+d);
        case 'U' : return uint8_t(f+b+e+c+d);
        case 'v' : return uint8_t(e+c+d);
        case 'V' : return uint8_t(f+b+e+c+d);
        case 'w' : return uint8_t(e+c+d);
        case 'W' : return uint8_t(f+b+e+c+d);
        case 'x' :
        case 'X' : return uint8_t(f+b+g+e+c);
        case 'y' :
        case 'Y' : return uint8_t(f+b+g+c+d);
        case 'z' :
        case 'Z' : return uint8_t(a+b+g+e+d);

        default : return 0;
    }
}

void SevenSegment::init()
{
    // System Setup register
    uint8_t wr = 0x21; // oscillator on
    i2c->write(addr, & wr, 1);
    // Display Register
    wr = 0x81; // display on, no blink
    i2c->write(addr, & wr, 1);
}

void SevenSegment::set_colon(bool state)
{
    colon = state;
}

const char *SevenSegment::next_seg(const char *text, uint8_t *_seg)
{
    ASSERT(_seg);
    ASSERT(text);

    char c = *text ? *text++ : 0;
    uint8_t s = seg(c);
    if (*text == '.')
    {
        s |= seg(*text++);
    }

    *_seg = s;
    return text;
}

void SevenSegment::write(const char *text)
{
    ASSERT(text);
    const uint8_t c = colon ? 0x02 : 0;

    uint8_t segs[4];

    text = next_seg(text, & segs[0]);
    text = next_seg(text, & segs[1]);
    text = next_seg(text, & segs[2]);
    text = next_seg(text, & segs[3]);

    uint8_t cmd[] = {
        0, // Display RAM address
        segs[0], 0,
        segs[1], 0,
        c, 0,
        segs[2], 0,
        segs[3], 0,
    };

    i2c->write(addr, cmd, sizeof(cmd));
}

}   //  namespace panglos

//  FIN
