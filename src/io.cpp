

// tiny printf library
// https://github.com/eyalroz/printf
#include "printf/printf.h"

#include "panglos/debug.h"

#include "panglos/mutex.h"

#include "panglos/io.h"

extern "C" {

void putchar_(char c)
{
    IGNORE(c);
    ASSERT(0);
}

}

namespace panglos {

    /*
     *
     */

void Out::tx_flush()
{
}

void In::set_event_handler(void (*fn)(Event *ev, void *arg), void *arg)
{
    IGNORE(fn);
    IGNORE(arg);
}

    /*
     *  Formatted output
     */

static void xputc(char c, void *arg)
{
    ASSERT(arg);
    FmtOut *fo = (FmtOut*) arg;
    fo->tx(c);
}

void FmtOut::tx(char c)
{
    out->tx(& c, 1);
}

int FmtOut::printf(const char *fmt, va_list va)
{
    if (!out) return 0;
    Lock lock(mutex);
    return vfctprintf(xputc, this, fmt, va);
}

int FmtOut::xprintf(void *arg, const char *fmt, va_list va)
{
    ASSERT(arg);
    FmtOut *fo = (FmtOut*) arg;
    return fo->printf(fmt, va);
}

int FmtOut::printf(const char *fmt, ...)
{
    va_list va;
    va_start(va, fmt);
    int n = printf(fmt, va);
    va_end(va);
    return n;
}

    /*
     *
     */

CharOut::CharOut(char *_data, int n)
:   data(_data),
    size(n),
    idx(0)
{
}

int CharOut::tx(const char* _data, int n)
{
    for (int i = 0; i < n; i++)
    {
        if (idx >= size)
        {
            return i;
        }

        data[idx] = *_data++;
        idx += 1;
        if (idx < size)
        {
            data[idx] = '\0';
        }
    }
    return n;        
}

void CharOut::reset()
{
    idx = 0;
    data[0] = '\0';
}

    /*
     *
     */

void LineOut::tx_flush()
{
    if (!idx) return;

    out->tx(data, idx);
    idx = 0;
}

int LineOut::tx(const char* _data, int n)
{
    for (int i = 0; i < n; i++)
    {
        if (idx >= size)
        {
            tx_flush();
        }

        char c = *_data++;

        data[idx] = c;
        idx += 1;

        if ((c == '\n') || (c == '\r'))
        {
            if (no_eol)
            {
                idx -= 1;
            }
            tx_flush();
        }
    }
    return n;        
}

LineOut::LineOut(int _size, Out *_out, bool _no_eol)
:   data(0),
    size(_size),
    idx(0),
    out(_out),
    no_eol(_no_eol)
{
    data = new char[size];
}

LineOut::~LineOut()
{
    tx_flush();
    delete[] data;
}

}   //  namespace panglos

//  FIN
