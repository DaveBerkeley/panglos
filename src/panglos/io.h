
#if !defined(__PANGLOS_IO__)
#define __PANGLOS_IO__

#include <stdarg.h>

#include "panglos/mutex.h"

    /*
     *
     */

namespace panglos {

class Out
{
public:
    virtual ~Out() { }

    virtual int tx(const char* data, int n) = 0;
    virtual void tx_flush();
};

class In
{
public:
    virtual ~In() { }

    virtual int rx(char* data, int n) = 0;

    class Event;

    virtual void set_event_handler(void (*fn)(Event *ev, void *arg), void *arg);
};

class IO : public Out, In { };

    /*
     *
     */

class Mutex;

class FmtOut
{
    Out *out;
    Mutex *mutex;
public:
    FmtOut(Out *_out, Mutex *m=0) : out(_out), mutex(m) { }

    void set(Out *_out) { out = _out; }
    Mutex *get_mutex() { return mutex; }

    int printf(const char *fmt, va_list va);
    int printf(const char *fmt, ...) __attribute__((format(printf, 2, 3)));

    void tx(char c);
};

}   //  namespace panglos

#endif  //  __PANGLOS_IO__

//  FIN
