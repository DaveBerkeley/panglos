
#include <stdint.h>

namespace panglos {

class Timer
{
public:
    virtual ~Timer() {}

    virtual void start(bool periodic) = 0;
    virtual void stop() = 0;

    typedef uint64_t Period;

    virtual void set_period(Period p) = 0;
    virtual void set_handler(void (*fn)(Timer *, void *), void *arg) = 0;

    static Timer *create();
};

}   //  namespace panglos
