
#if !defined(__EVENT_H__)
#define __EVENT_H__

#include <stdint.h>

#include "mutex.h"
#include "timer.h"

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
    Rescheduler *rescheduler;
public:
    Event *events;

private:
    Event* _remove(Event *ev, Mutex *mutex);

public:
    bool add(Event *ev);
    Event* remove(Event *ev);
    void reschedule(d_timer_t dt);
public:
    EventQueue(Rescheduler *r);
    ~EventQueue();

    void wait(Semaphore *s, d_timer_t time);
    d_timer_t check();

    void run();
};

// Global event queue
extern EventQueue event_queue;

}   //  namespace panglos

#endif // __EVENT_H__

//  FIN
