
#if !defined(NO_TESTS)

#include <pthread.h>
#include <semaphore.h>

#include <panglos/debug.h>
#include <panglos/event.h>

#include "mock.h"

namespace panglos {

class LinuxMutex : public Mutex
{
    pthread_mutex_t mutex;

public:
    LinuxMutex()
    {
        int err = pthread_mutex_init(& mutex, 0);
        ASSERT(err == 0);
    }

private:
    virtual ~LinuxMutex()
    {
        int err = pthread_mutex_destroy(& mutex);
        ASSERT(err == 0);
    };

    virtual void lock()
    {
        int err = pthread_mutex_lock(& mutex);
        ASSERT(err == 0);
    }

    virtual void unlock()
    {
        int err = pthread_mutex_unlock(& mutex);
        ASSERT(err == 0);
    }
};

Mutex* Mutex::create()
{
    return new LinuxMutex();
}

Mutex* Mutex::create_critical_section()
{
    return new LinuxMutex();
}

}   //  namespace panglos

    /*
     *
     */

LinuxSemaphore::LinuxSemaphore()
: hook(0), posted(0)
{
    int err = sem_init(& semaphore, 0, 0);
    ASSERT(err == 0);
}

LinuxSemaphore::~LinuxSemaphore()
{
    int err = sem_destroy(& semaphore);
    ASSERT(err == 0);
}

void LinuxSemaphore::post()
{
    if (hook)
    {
        hook->post(this);
        return;
    }

    posted += 1;
    int err = sem_post(& semaphore);
    ASSERT(err == 0);
}

void LinuxSemaphore::wait()
{
    int err = sem_wait(& semaphore);
    ASSERT(err == 0);
    posted -= 1;
}

void LinuxSemaphore::set_hook(panglos::PostHook *_hook)
{
    hook = _hook;
}

namespace panglos {

Semaphore* Semaphore::create()
{
    return new LinuxSemaphore();
}

    /*
     *
     */

static timer_t cycles;

void timer_irq()
{
    cycles += 1;

    // TODO : Call the EventQueue handler
}

    /*
     *
     */

void *get_task_id()
{
    return (void*) pthread_self();
}

}   //  namespace

#endif // NO_TESTS

//  FIN
