
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

uint8_t SevenSegment::seg(char data)
{
    // https://en.wikipedia.org/wiki/Seven-segment_display
    const uint8_t a = 0x01;
    const uint8_t b = 0x02;
    const uint8_t c = 0x04;
    const uint8_t d = 0x08;
    const uint8_t e = 0x10;
    const uint8_t f = 0x20;
    const uint8_t g = 0x40;
    const uint8_t dp = 0x80;
    switch (data)
    {
        case 0x01 : return a;
        case 0x02 : return b;
        case 0x03 : return c;
        case 0x04 : return d;
        case 0x05 : return e;
        case 0x06 : return f;
        case 0x07 : return g;

        case '0' : return a+b+c+d+e+f;
        case '1' : return b+c;
        case '2' : return a+b+g+e+d;
        case '3' : return a+b+g+c+d;
        case '4' : return f+b+g+c;
        case '5' : return a+f+g+c+d;
        case '6' : return a+f+g+e+c+d;
        case '7' : return a+b+c;
        case '8' : return a+f+b+g+e+c+d;
        case '9' : return a+f+b+g+c+d;

        case ' ' : return 0;
        case '.' : return dp;
        case ',' : return dp;
        case '+' : return b+g+c;
        case '-' : return g;
        case '=' : return g+d;
        case '"' : return f+b;
        case '\'': return b;
        case '!' : return b+c+dp;
        case '|' : return f+e;
        case '(' :
        case '{' :
        case '[' : return a+f+e+d;
        case ')' :
        case '}' :
        case ']' : return a+b+c+d;
        case '?' : return a+b+g+e;
        case '*' : return f+b+g+e+c;
        case '#' : return a+f+b+g;
        case '/' : return b+g+e;
        case '\\': return f+g+c;
        case ':' : return g+d;
        case ';' : return g+d;
        case '$' : return a+f+g+c+d;
        case '%' : return b+g+e;
        case '&' : return a+f+b+g+e;
        case '_' : return d;
        case '^' : return a+f+b;
        case '`' : return f;
        case '@' : return a+f+b+g+e+d;
        case '>' : return g+c+d;
        case '<' : return g+e+d;
        case '~' : return a;

        case 'a' :
        case 'A' : return a+f+b+g+e+c;
        case 'b' : return f+g+e+c+d;
        case 'B' : return f+g+e+c+d;
        case 'c' : return g+e+d;
        case 'C' : return a+f+e+d;
        case 'd' : return b+g+e+c+d;
        case 'D' : return a+f+b+e+c+d;
        case 'e' :
        case 'E' : return a+f+g+e+d;
        case 'f' :
        case 'F' : return a+f+g+e;
        case 'g' :
        case 'G' : return a+f+b+g+c+d;
        case 'h' : return f+g+e+c;
        case 'H' : return f+b+g+e+c;
        case 'i' : 
        case 'I' : return b+c;
        case 'j' :
        case 'J' : return b+c+d;
        case 'k' : return a+f+g+e+c;
        case 'K' : return f+b+g+e+c;
        case 'l' :
        case 'L' : return f+e+d;
        case 'm' : return g+e+c;
        case 'M' : return a+f+b+e+c;
        case 'n' : return g+e+c;
        case 'N' : return a+f+b+e+c;
        case 'o' : return g+e+c+d;
        case 'O' : return a+f+b+e+c+d;
        case 'p' :
        case 'P' : return a+f+b+g+e;
        case 'q' :
        case 'Q' : return a+f+b+g+c;
        case 'r' :
        case 'R' : return g+e;
        case 's' :
        case 'S' : return a+f+g+c+d;
        case 't' : 
        case 'T' : return f+g+e+d;
        case 'u' : return e+c+d;
        case 'U' : return f+b+e+c+d;
        case 'v' : return e+c+d;
        case 'V' : return f+b+e+c+d;
        case 'w' : return e+c+d;
        case 'W' : return f+b+e+c+d;
        case 'x' :
        case 'X' : return f+b+g+e+c;
        case 'y' :
        case 'Y' : return f+b+g+c+d;
        case 'z' :
        case 'Z' : return a+b+g+e+d;

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
