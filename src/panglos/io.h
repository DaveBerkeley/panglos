
#if !defined(__PANGLOS_IO__)
#define __PANGLOS_IO__

#include <stdarg.h>
#include <stdlib.h>

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

class IO : public Out, public In { };

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

    static int xprintf(void *, const char *fmt, va_list va);
};

    /*
     *
     */

class CharOut : public Out
{
    char *data;
    int size;
    int idx;

    virtual int tx(const char* _data, int n) override;

public:
    CharOut(char *_data, int n);

    void reset();
};

    /*
     *
     */

class LineOut : public Out
{
    char *data;
    int size;
    int idx;
    Out *out;
    bool no_eol;

    virtual int tx(const char* _data, int n) override;

public:
    LineOut(int _size, Out *_out, bool _no_eol);
    ~LineOut();

    virtual void tx_flush() override;
};

    /*
     *
     */

class CharIn : public In
{
public:
    const char *s;
    size_t size;
    size_t idx;

    CharIn(const char *_s, size_t _size=0);

    virtual int rx(char* data, int n) override;
};

    /*
     *
     */

class LineReader : public In
{
    In *input;
    const char *delimit;

public:
    LineReader(In *_input, const char *_delimit="\r\n");

    virtual int rx(char* data, int n) override;

    int strip(char *data, int n);
};


}   //  namespace panglos

#endif  //  __PANGLOS_IO__

//  FIN
