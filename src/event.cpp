
#include "panglos/debug.h"

#include "panglos/list.h"
#include "panglos/semaphore.h"
#include "panglos/event.h"

namespace panglos {

    /*
    *   Event list manipulation
    */

static inline int timer_cmp(timer_t t1, timer_t t2)
{
    // use signed arithmetic, to allow the time to wrap
    const int32_t s1 = t1;
    const int32_t s2 = t2;
    return s2 - s1;
}

static int event_cmp(Event* event1, Event* event2)
{
    return timer_cmp(event1->time, event2->time);
}

    /*
     *
     */

EventQueue::EventQueue(Rescheduler *r, Mutex *m)
: mutex(m), delete_mutex(0), rescheduler(r), events(Event::next_fn)
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

    /*
     *
     */

//#define EVENT_SHOW

#if defined(EVENT_SHOW)

static int visit(Event *e, void *arg)
{
    ASSERT(arg);
    Output *out = (Output*) arg;
    out->printf("%#x ", e->time);
    return 0;
}

static void show(const char *label, List<Event*> *events)
{
    // show the state of the queue
    Buffers bs;
    BufferOutput bo(& bs, 128);
    events->visit(visit, & bo, 0);
    char buff[64];
    while (true)
    {
        int n = bs.read((uint8_t*) buff, sizeof(buff)-1);
        if (n == 0)
        {
            break;
        }
        buff[n] = '\0';
        PO_DEBUG("%s: [ %s]", label, buff);
    }
}

#else // EVENT_SHOW

#define show(x,y)

#endif // EVENT_SHOW

    /*
     *
     */

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

    events.add_sorted(ev, event_cmp, 0);
    // return true if the new event is now at the head of the queue
    const bool first = (ev == events.head);
    show(__FUNCTION__, & events);
    return first;
}

Event* EventQueue::_remove(Event *ev, Mutex *mutex)
{
    return events.remove(ev, mutex) ? ev : 0;
}

Event* EventQueue::remove(Event *ev)
{
    return _remove(ev, mutex);
}

bool EventQueue::waiting(Event *ev)
{
    return events.has(ev, mutex);
}

    /*
     *
     */

d_timer_t EventQueue::check()
{
    const timer_t now = timer_now();

    Lock lock(mutex);

    while (events.head)
    {
        Event *event = events.head;
        const int diff = timer_cmp(now, event->time);

        if (diff > 0)
        {
            // return the time until the next event
            return diff;
        }

        show(__FUNCTION__, & events);

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

void EventQueue::wait_absolute(Semaphore *semaphore, timer_t time)
{
    ASSERT(semaphore);

    const timer_t now = panglos::timer_now();
    if (now >= time)
    {
        // no point in waiting
        return;
    }

    Event event(semaphore, time);

    const bool first = add(& event);

    if (first)
    {
        // may need to reschedule the timer manager task / thread
        reschedule(time - now);
    }

    event.semaphore->wait();
    // event is removed from the queue by the event manager if it times out
    // but not if the semaphore is signalled elsewhere
    _remove(& event, mutex);
}

    /*
     *
     */

void EventQueue::wait(Semaphore *semaphore, d_timer_t d_time)
{
    if (d_time == 0)
    {
        // no point waiting
        return;
    }

    const timer_t now = panglos::timer_now();
    return wait_absolute(semaphore, now + d_time);
}

    /*
     *
     */

void EventQueue::run()
{
    PO_DEBUG("");
    // start the global timer on this task
    timer_init();

    while (true)
    {
        d_timer_t next = event_queue.check();
        // if next==0, wait arbitrary time.
        // the next reschedule will fix this.
        timer_wait(next ? next : 10000);
    }
}

}   //  namespace

//  FIN
