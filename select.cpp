
#include <stdlib.h>
#include <string.h>

#include "debug.h"

#include "list.h"
#include "msg_queue.h"
#include "select.h"

namespace panglos {

    /*
     *
     */

Select::Select(int _size)
: mutex(0), semaphore(0), queue(Semaphore::next_fn), captured(0), size(_size) // semaphores(next_fn)
{
    // Semaphores may be posted in an interrupt, so use critical section
    mutex = Mutex::create_critical_section();
    semaphore = Semaphore::create();

    captured = (Semaphore**) malloc(sizeof(Semaphore*) * size);
    memset(captured, 0, sizeof(Semaphore*) * size);
}

Select::~Select()
{
    // remove all semaphores in the list
    for (int i = 0; i < size; i++)
    {
        if (captured[i])
        {
            remove(captured[i]);
        }
    }

    delete semaphore;
    delete mutex;
    free(captured);
}

void Select::post(Semaphore *s)
{
    Lock lock(mutex);

    if (queue.find(s, 0))
    {
        // Sem is already notified
        return;
    }
 
    queue.push_tail(s, 0);
    semaphore->post();
}

void Select::add(Semaphore *s)
{
    ASSERT(s);
    s->set_hook(this);

    for (int i = 0; i < size; i++)
    {
        if (!captured[i])
        {
            captured[i] = s;
            return;
        }
    }

    ASSERT(0);
}

void Select::remove(Semaphore *s)
{
    ASSERT(s);
    s->set_hook(0);

    for (int i = 0; i < size; i++)
    {
        if (captured[i] == s)
        {
            captured[i] = 0;
            return;
        }
    }

    ASSERT(0);
}

Semaphore *Select::wait()
{
    if (queue.empty())
    {
        semaphore->wait();
    }

    Semaphore *s = queue.pop_head(mutex);
    return s;
}

void Select::visit(int (*fn)(Semaphore*, void*), void *arg)
{
    queue.visit(fn, arg, mutex);    
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
