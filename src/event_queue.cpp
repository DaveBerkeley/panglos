
#include <gtest/gtest.h>

#include <panglos/debug.h>

#include <panglos/mutex.h>
#include <panglos/time.h>

#include <panglos/event_queue.h>

namespace panglos {

static int cmp_t(Time::tick_t t1, Time::tick_t t2)
{
    const int diff = int(t2 - t1);
    //PO_DEBUG("%#x %#x %d", t1, t2, diff);
    return diff;
}

int EvQueue::Event::cmp(Event *ev1, Event* ev2)
{
    return cmp_t(ev1->when, ev2->when);
}

    /*
     *
     */

EvQueue::EvQueue()
:   mutex(0),
    events(Event::next_fn)
{
    mutex = panglos::Mutex::create();
}

EvQueue::~EvQueue()
{
    delete mutex;
}

void EvQueue::add(EvQueue::Event *ev)
{
    //PO_DEBUG("%p %#x", ev, ev->when);
    events.add_sorted(ev, Event::cmp, mutex);
}

bool EvQueue::del(EvQueue::Event *ev)
{
    //PO_DEBUG("%p", ev);
    return events.remove(ev, mutex);
}

void EvQueue::reschedule(EvQueue::Event *ev, Time::tick_t t)
{
    //PO_DEBUG("%p %d", ev, t);
    del(ev);
    ev->when = t;
    add(ev);
}

EvQueue::Event *EvQueue::pop(Time::tick_t t)
{
    panglos::Lock lock(mutex);
    EvQueue::Event *ev = events.head;

    if (!ev)
    {
        return 0;
    }

    int diff = cmp_t(ev->when, t);
    //PO_DEBUG("diff=%d", diff);

    if (diff < 0)
    {
        return 0;
    }

    return events.pop(0);
}

bool EvQueue::run(Time::tick_t t)
{
    bool found = false;

    while (true)
    {
        EvQueue::Event *ev = pop(t);
        if (!ev)
        {
            return found;
        }
        ev->run(this);
        found = true;
    }
}

}   //  namespace panglos

//  FIN
