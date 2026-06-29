
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

    Watched(Thread *thread)
    :   Task(thread),
        next(0)
    {
    }
};

    /*
     *
     */

class DummyThread : public panglos::Thread
{
    virtual void start(void (*)(void *arg), void *, int ) override
    {
        ASSERT(0);
    }

    virtual void join() override
    {
        ASSERT(0);
    }
    
    virtual const char *get_name() override
    {
        return "system";
    }
};

static DummyThread dummy_thread;

    /*
     *
     */

class _Watchdog : public Watchdog
{
    List<Watched*> tasks;
    Mutex *mutex;
    Mutex *mutex_tidy;

    virtual void poll(Time::tick_t holdoff, Thread *thread) override
    {
        //PO_DEBUG("");
        if (!thread)
        {
            thread = Thread::get_current();
        }
        if (!thread)
        {
            thread = & dummy_thread;
        }

        Lock lock(mutex);

        Watched *w = find(thread, 0);
        if (!w)
        {
            PO_DEBUG("create new Watched{}");
            w = new Watched(thread);
            tasks.push(w, 0);
        }

        w->last = Time::get();
        w->holdoff = holdoff;
        w->alarmed = false;
    }

    virtual void remove(Thread *thread=0) override
    {
        PO_DEBUG("");
        Lock lock(mutex);

        Watched *w = find(thread, 0);
        if (w)
        {
            PO_DEBUG("removed");
            tasks.remove(w, 0);
            delete w;
        }
    }

    struct Check
    {
        Watched *w;
    };

    static int visit(Watched *w, void *arg)
    {
        //PO_DEBUG("");
        ASSERT(arg);
        struct Check *check = (struct Check *) arg;

        if (w->alarmed)
        {
            return false;
        }

        if (!Time::elapsed(w->last, w->holdoff))
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
        //PO_DEBUG("");
        struct Check check = { .w = 0, };
        tasks.visit(visit, & check, mutex);
        return check.w;
    }

    virtual bool expired(Thread *thread=0) override
    {
        Watched *w = find(thread, mutex);
        return w ? w->alarmed : false;
    }

    static int match(Watched*w, void *arg)
    {
        ASSERT(w);
        ASSERT(arg);
        Thread *thread = (Thread*) arg;
        return (thread == w->thread) ? 1 : 0;
    }

    Watched *find(Thread *thread, Mutex *mutex)
    {
        ASSERT(thread);
        Watched *w = tasks.find(match, thread, mutex);
        return w;
    }

public:
    _Watchdog(Mutex *m)
    :   tasks(Watched::get_next),
        mutex(m),
        mutex_tidy(0)
    {
        if (!mutex)
        {
            mutex_tidy = mutex = Mutex::create();
        }
    }

    ~_Watchdog()
    {
        while (!tasks.empty())
        {
            Watched *w = tasks.pop(mutex);
            delete w;
        }
        delete mutex_tidy;
    }
};

    /*
     *
     */

Watchdog *Watchdog::create(Mutex *mutex)
{
    return new _Watchdog(mutex);
}

}   //  namespace panglos

//  FIN
