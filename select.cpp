
#include "debug.h"

#include "msg_queue.h"
#include "select.h"

namespace panglos {

    /*
     *
     */

void Select::post(Semaphore *s)
{
    queue->put(s);
}

void Select::add(Semaphore *s)
{
    ASSERT(s);
    s->set_hook(this);

    {
        Lock lock(mutex);
        list->push_front(s);
    }
}

void Select::remove(Semaphore *s)
{
    ASSERT(s);
    s->set_hook(0);

    {
        Lock lock(mutex);
        list->remove(s);
    }
}

Semaphore *Select::wait()
{
    return queue->wait();
}

Select::Select()
: deque(0), mutex(0), semaphore(0), list(0)
{
    deque = new Queue::Deque;
    // Semaphores may be posted in an interrupt, so use critical section
    mutex = Mutex::create_critical_section();
    semaphore = Semaphore::create();
    queue = new Queue(deque, mutex, semaphore);

    list = new List;

    // send a null message through now, to ensure that
    // the deque is initialised (by the first message)
    queue->put(0);
    queue->wait();
}

Select::~Select()
{
    // remove all semaphores in the list
    while (!list->empty())
    {
        Semaphore *s = list->front();
        remove(s);
    }

    delete list;
    delete queue;
    delete semaphore;
    delete mutex;
    delete deque;
}

Semaphore *Select::wait(EventQueue *eq, timer_t timeout)
{
    ASSERT(eq);
    Semaphore *waiter = Semaphore::create();
    // add waiter to the semaphores we're waiting on
    add(waiter);

    // create an Event to notify the event queue
    Event event(waiter, timeout);
    eq->add(& event);

    // wait for any of the semaphores
    Semaphore *s = wait();
    // remove the waiter from the event queue
    eq->remove(& event);

    // if wait() returned waiter, we timed out
    if (s == waiter)
    {
        // timed out
        s = 0;
    }

    remove(waiter);
    delete waiter;

    return s;
}

}   //  namespace panglos

//  FIN
