
#include "debug.h"

#include "dispatch.h"

namespace panglos {

Dispatch::Nowt Dispatch::nowt;

    /*
     *
     */

Dispatch::Dispatch()
: deque(0), mutex(0), semaphore(0), queue(0)
{
    deque = new Queue::Deque;
    // needs to be irq_safe, not just thread safe
    mutex = Mutex::create_critical_section();
    semaphore = Semaphore::create();
    queue = new Queue(deque, mutex, semaphore);

    // kick the queue off with a nowt task.
    // This avoids the deque being initialised (uses malloc()) in an interrupt.
    put(& nowt);
}

Dispatch::~Dispatch()
{
    delete queue;
    delete semaphore;
    delete mutex;
    delete deque;
}

/// called from within irq context
void Dispatch::put(Callback *cb)
{
    queue->put(cb);
}

/// main dispatch loop
void Dispatch::run()
{
    PO_DEBUG("");

    while (true)
    {
        Callback *cb = queue->wait();
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
