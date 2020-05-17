
#if !defined(__EVENT_H__)
#define __EVENT_H__

#include <stdint.h>

#include "mutex.h"
#include "timer.h"
#include "list.h"

namespace panglos {

class Event
{
public:
    struct Event *next;
    Semaphore *semaphore;
    timer_t time;
public:
    Event(Semaphore *s, timer_t t)
    : next(0), semaphore(s), time(t)
    {
    }

    static Event **next_fn(Event *ev) { return & ev->next; }
};

    /*
     *
     */

class EventQueue;

class Rescheduler
{
public:
    virtual ~Rescheduler() {}
    virtual void reschedule(EventQueue *eq, d_timer_t dt) = 0;
};

class EventQueue
{
private:
    Mutex *mutex;
    Mutex *delete_mutex;
    Rescheduler *rescheduler;
public:
    List<Event*> events;

private:
    Event* _remove(Event *ev, Mutex *mutex);

public:
    bool add(Event *ev);
    Event* remove(Event *ev);
    void reschedule(d_timer_t dt);
public:
    EventQueue(Rescheduler *r, Mutex *mutex=0);
    ~EventQueue();

    void wait(Semaphore *s, d_timer_t time);
    void wait_absolute(Semaphore *s, timer_t time);
    d_timer_t check();

    void run();
};

// Global event queue
extern EventQueue event_queue;

}   //  namespace panglos

#endif // __EVENT_H__

//  FIN
