
#include <unistd.h>
#include <time.h>

#include "panglos/debug.h"
#include "panglos/time.h"

namespace panglos {

static Time::tick_t fake_time = 0;

void Time::sleep(int secs)
{
    ASSERT(secs >= 0);
    ::sleep(unsigned(secs));
}

void Time::msleep(int msecs)
{
    ASSERT(msecs >= 0);
    usleep(useconds_t(1000 * msecs));
}

Time::tick_t Time::get()
{
    if (fake_time)
    {
        return fake_time;
    }

    struct timespec tp;
    int err = clock_gettime(CLOCK_REALTIME, & tp);
    ASSERT(err == 0);

    __time_t ms = tp.tv_sec * 1000;
    ms += tp.tv_nsec / 1000000;

    return tick_t(ms);
}

void Time::set(Time::tick_t t)
{
    fake_time = t;
}

}   //  panglos

// FIN

