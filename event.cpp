
#include "debug.h"

#include "list.h"
#include "event.h"

namespace panglos {

    /*
    *   Event list manipulation
    */

static pList* pnext_event(pList v)
{
    Event *event = (Event*) v;
    return (pList*) & event->next;
}

static inline int timer_cmp(timer_t t1, timer_t t2)
{
    // use signed arithmetic, to allow the time to wrap
    const int32_t s1 = t1;
    const int32_t s2 = t2;
    return s2 - s1;
}

static int event_cmp(const pList ev1, const pList ev2)
{
    Event *event1 = (Event*) ev1;
    Event *event2 = (Event*) ev2;

    return timer_cmp(event1->time, event2->time);
}

    /*
     *
     */

EventQueue::EventQueue(Rescheduler *r, Mutex *m)
: mutex(m), delete_mutex(0), rescheduler(r), events(0)
{
    if (!mutex)
    {
        delete_mutex = mutex = Mutex::create();
    }
}

EventQueue::~EventQueue()
{
    delete delete_mutex;
}

void EventQueue::reschedule(d_timer_t dt)
{
    if (rescheduler)
    {
        rescheduler->reschedule(this, dt);
    }
}

bool EventQueue::add(Event *ev)
{
    Lock lock(mutex);

    list_add_sorted((pList*) & events, (pList) ev, pnext_event, event_cmp, 0);
    // return true if the new event is now at the head of the queue
    const bool first = (ev == events);
    return first;
}

Event* EventQueue::_remove(Event *ev, Mutex *mutex)
{
    return list_remove((pList*) & events, (pList) ev, pnext_event, mutex) ? ev : 0;
}

Event* EventQueue::remove(Event *ev)
{
    return _remove(ev, mutex);
}

    /*
     *
     */

d_timer_t EventQueue::check()
{
    const timer_t now = timer_now();

    Lock lock(mutex);

    while (events)
    {
        Event *event = events;
        const int diff = timer_cmp(now, event->time);

        if (diff > 0)
        {
            // return the time until the next event
            return diff;
        }

        // Save the semaphore before removing the event from the list
        Semaphore *semaphore = event->semaphore;
        // The event must be removed before posting the semaphore.
        // It will no longer be valid after the post.
        _remove(event, 0);
        semaphore->post();
    }

    return 0;
}

    /*
     *
     */

void EventQueue::wait(Semaphore *semaphore, d_timer_t d_time)
{
    ASSERT(semaphore);

    if (d_time == 0)
    {
        // no point waiting
        return;
    }

    const timer_t now = panglos::timer_now();

    Event event(semaphore, now + d_time);

    const bool first = add(& event);

    if (first)
    {
        // may need to reschedule the timer manager task / thread
        reschedule(d_time);
    }

    event.semaphore->wait();
    // event is removed from the queue by the event manager if it times out
    // but not if the semaphore is signalled elsewhere
    _remove(& event, mutex);
}

void EventQueue::run()
{
    PO_DEBUG("");
    // start the global timer on this task
    timer_init();

    while (true)
    {
        d_timer_t next = event_queue.check();
        timer_wait(next);
    }
}

}   //  namespace

//  FIN
