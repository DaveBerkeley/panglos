

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

}   //  namespace panglos

//  FIN
