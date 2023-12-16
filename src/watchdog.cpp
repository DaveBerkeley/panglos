
#include <stdint.h>
#include <stdlib.h>

#include "panglos/debug.h"

#include "panglos/mutex.h"
#include "panglos/list.h"
#include "panglos/thread.h"
#include "panglos/watchdog.h"

namespace panglos {

    /*
     *
     */

class Watched : public Watchdog::Task
{
public:
    Watched *next;
    Time::tick_t last;
    Time::tick_t holdoff;
    bool alarmed;

    static Watched** get_next(Watched *w) { return & w->next; }

    Watched(Thread *t)
    :   Task(t),
        next(0),
        holdoff(0),
        alarmed(false)
    {
        last = Time::get();
    }
};

    /*
     *
     */

class _Watchdog : public Watchdog
{
    List<Watched*> tasks;
    Mutex *mutex;
    Time::tick_t period;

    virtual void poll() override
    {
        PO_DEBUG("");
        Thread *thread = Thread::get_current();
        Watched *w = find(thread);
        if (!w)
        {
            PO_DEBUG("create new Watched{}");
            w = new Watched(thread);
            tasks.push(w, mutex);
        }
        else
        {
            w->last = Time::get();
            if (w->holdoff)
            {
                // end holdoff
                w->holdoff = 0;
            }
            w->alarmed = false;
        }
    }

    virtual void remove(Thread *thread=0) override
    {
        PO_DEBUG("");
        Watched *w = find(thread);
        if (w)
        {
            PO_DEBUG("removed");
            tasks.remove(w, mutex);
            delete w;
        }
    }

    virtual void holdoff(Time::tick_t period, Thread *thread=0) override
    {
        PO_DEBUG("%d %p", period, thread);
    }

    struct Check
    {
        Time::tick_t period;
        Watched *w;
    };

    static int visit(Watched *w, void *arg)
    {
        PO_DEBUG("");
        ASSERT(arg);
        struct Check *check = (struct Check *) arg;

        if (w->alarmed)
        {
            return false;
        }

        Time::tick_t cmp = w->holdoff ? w->holdoff : w->last;

        if (!Time::elapsed(cmp, check->period))
        {
            return 0;
        }

        PO_DEBUG("Elapsed '%s'", w->thread->get_name());
        check->w = w;
        w->alarmed = true;
        return 1;
    }

    virtual Task *check_expired() override
    {
        PO_DEBUG("");
        struct Check check = { .period = period, };
        tasks.visit(visit, & check, mutex);
        return check.w;
    }

    virtual bool expired(Thread *thread=0) override
    {
        Watched *w = find(thread);
        return w ? w->alarmed : false;
    }

    static int match(Watched*w, void *arg)
    {
        ASSERT(w);
        ASSERT(arg);
        Thread *thread = (Thread*) arg;
        return (thread == w->thread) ? 1 : 0;
    }

    Watched *find(Thread *thread)
    {
        if (!thread)
        {
            thread = Thread::get_current();
        }
        Watched *w = tasks.find(match, thread, mutex);
        return w;
    }

public:
    _Watchdog(Time::tick_t _period)
    :   tasks(Watched::get_next),
        mutex(0),
        period(_period)
    {
        mutex = Mutex::create();
    }

    ~_Watchdog()
    {
        while (!tasks.empty())
        {
            Watched *w = tasks.pop(mutex);
            delete w;
        }
        delete mutex;
    }
};

    /*
     *
     */

Watchdog *Watchdog::create(Time::tick_t period)
{
    return new _Watchdog(period);
}

}   //  namespace panglos

//  FIN
