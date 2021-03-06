
#if !defined(__SPRINTF_H__)
#define __SPRINTF_H__

#include <stdarg.h>

#include "mutex.h"

namespace panglos {

// TODO : move this to something akin to stdio.h for the library
class Output
{
public:
    Mutex *mutex;
public:
    Output() : mutex(0) { }
    virtual ~Output() { }

    virtual int _putc(char c) = 0;
    virtual int _puts(const char *s, int n);
    virtual void flush() { }

    virtual int printf(const char* fmt, ...);

    static Output *create_buffered(Output *out, int size);
};

    /*
     *
     */

class Format {
public:
    char flags;
    char leading;
    int width;
    int precision;
    char length[3];
    char specifier;

    Format();
    int get_num(const char **fmt, va_list va);
    void get(const char **fmt, va_list va);
};

    /*
     *
     */

int _print_num(Output *output, const Format *format, unsigned long int number, int base, bool negative);

//  API
int xvprintf(Output *output, const char* fmt, va_list va);
int xprintf(Output *output, const char* fmt, ...);

}   //  namespace panglos

#endif // __SPRINTF_H__

//  FIN
