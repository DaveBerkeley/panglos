
#include <string.h>
#include <ctype.h>

// tiny printf library
// https://github.com/eyalroz/printf
#include "printf/printf.h"

#include "panglos/debug.h"

#include "panglos/mutex.h"

#include "panglos/io.h"

extern "C" {

// used by printf library
void __attribute__((weak)) putchar_(char c)
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
    PO_WARNING("Not implemented");
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
    data = new char[size_t(size)];
}

LineOut::~LineOut()
{
    tx_flush();
    delete[] data;
}

    /*
     * 
     */

CharIn::CharIn(const char *_s, size_t _size)
:   s(_s),
    size(_size),
    idx(0)
{
    if (s && !size)
    {
        size = strlen(s);
    }
}

int CharIn::rx(char* data, int n)
{
    size_t todo = size - idx;
    if (!todo)
    {
        return 0;
    }
    size_t block = (((size_t)n) > todo) ? todo : size_t(n);
    memcpy(data, & s[idx], block);
    idx += block;
    return (int) block;
}

    /*
     *
     */

LineReader::LineReader(In *_input, const char *_delimit)
:   input(_input),
    delimit(_delimit)
{
}

int LineReader::rx(char* data, int n)
{
    for (int i = 0; i < n; i += 1)
    {
        int s = input->rx(& data[i], 1);
        if (!s)
        {
            return i;
        }
        if (strchr(delimit, data[i]))
        {
            return i+1;
        }
    }
    return n;
}

int LineReader::strip(char *data, int n)
{
    while (n > 0)
    {
        if (!strchr(delimit, data[n-1]))
        {
            return n;
        }
        data[n-1] = '\0';
        n -= 1;
    }
    return n;
}

    /*
     *
     */

void dump(panglos::Out *out, const uint8_t *data, size_t sz, bool show_idx)
{
    panglos::FmtOut fmt(out);
    const int row = 16;

    fmt.printf("Dump %p\r\n", data);
    for (size_t i = 0; i < sz; i += row)
    {   
        if (show_idx)
        {
            fmt.printf("%04X:", (int) i);
        }
        else
        {
            fmt.printf("%p:", & data[i]);
        }

        for (int j = 0; j < row; j++)
        {
            const char *mid = (j % 8) ? "" : " ";
            if ((i + j) < sz)
            {
                fmt.printf("%s %02X", mid, (int) data[i+j]);
            }
            else
            {
                fmt.printf("%s   ", mid);
            }
        }

        fmt.printf("  |");

        for (int j = 0; j < row; j++)
        {
            if ((i + j) < sz)
            {
                char c = (char) data[i+j];
                fmt.printf("%c", isprint(c) ? c : '.');
            }
            else
            {
                fmt.printf(" ");
            }
        }

        fmt.printf("|\r\n");
    }
}


}   //  namespace panglos

//  FIN
