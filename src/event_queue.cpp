
#include <stdint.h>

//#include <panglos/debug.h>

#include <panglos/time.h>
#include <panglos/event_queue.h>

namespace panglos {

template<>
int _EvQueue<Time::tick_t>::Event::cmp_t(Time::tick_t t1, Time::tick_t t2)
{
    const int diff = int(t2 - t1);
    //PO_DEBUG("%#x %#x %d", t1, t2, diff);
    return diff;
}

}   //  namespace panglos

//  FIN
