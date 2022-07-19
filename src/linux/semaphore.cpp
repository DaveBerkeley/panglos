
//#include <pthread.h>
#include <semaphore.h>

#include <panglos/debug.h>
#include <panglos/semaphore.h>

using namespace panglos;

    /*
     *
     */

class LinuxSemaphore : public Semaphore
{
    PostHook *hook;
public:
    sem_t semaphore;
    int posted;

    LinuxSemaphore();
 
    virtual ~LinuxSemaphore();
    virtual void post();
    virtual void wait();
    virtual void set_hook(PostHook *hook);
};

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

void LinuxSemaphore::set_hook(PostHook *_hook)
{
    hook = _hook;
}

namespace panglos {

Semaphore* Semaphore::create()
{
    return new LinuxSemaphore();
}

}

//  FIN
