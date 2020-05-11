
#include "debug.h"

#include "dispatch.h"

namespace panglos {

    /*
     *
     */

Dispatch::Dispatch()
: deque(Callback::next_fn), mutex(0), semaphore(0), dead(false)
{
    // needs to be irq_safe, not just thread safe
    mutex = Mutex::create_critical_section();
    semaphore = Semaphore::create();
}

Dispatch::~Dispatch()
{
    delete semaphore;
    delete mutex;
}

/// potentialy called from within irq context
void Dispatch::put(Callback *cb)
{
    ASSERT(cb);
    deque.push_tail(cb, mutex);
    semaphore->post();
}

void Dispatch::kill()
{
    dead = true;
    semaphore->post();
}

/// main dispatch loop
void Dispatch::run()
{
    PO_DEBUG("");

    while (!dead)
    {
        semaphore->wait();

        Callback *cb = (Callback*) deque.pop_head(mutex);
        if (!cb)
        {
            break;
        }

        //PO_DEBUG("debug=%s", cb->debug);
        cb->execute();
    }

    PO_DEBUG("leave");
}

Dispatch::FnArg::FnArg(void (*_fn)(void *arg), void *_arg, const char *_debug)
: Callback(_debug), fn(_fn), arg(_arg)
{
}

void Dispatch::FnArg::execute()
{
    ASSERT(fn);
    fn(arg);
}

}   //  namespace panglos

// FIN
