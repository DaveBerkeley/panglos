
#pragma once

#include "panglos/time.h"

namespace panglos {

class Thread;
class Mutex;

class Watchdog
{
public:
    class Task
    {
    public:
        Thread *thread;
        Task(Thread *t) : thread(t) { }
        virtual ~Task() { }
    };

    virtual ~Watchdog() {}
    virtual void poll(Time::tick_t holdoff, Thread *thread=0) = 0;
    virtual void remove(Thread *thread=0) = 0;
    virtual bool expired(Thread *thread=0) = 0;

    virtual Task *check_expired() = 0;

    static Watchdog *create(Mutex *mutex=0);
};

}   //  namespace panglos

//  FIN
