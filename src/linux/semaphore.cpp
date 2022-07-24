
#include <atomic>
#include <semaphore.h>

#include <panglos/debug.h>
#include <panglos/semaphore.h>

using namespace panglos;

    /*
     *
     */

class LinuxSemaphore : public Semaphore
{
public:
    sem_t semaphore;
    std::atomic<int> posted;

    LinuxSemaphore(unsigned int initial);
 
    virtual ~LinuxSemaphore();
    virtual void post() override;
    virtual void wait() override;
    virtual void wait_timeout(int ticks) override;
};

    /*
     *
     */

LinuxSemaphore::LinuxSemaphore(unsigned int initial)
: posted(0)
{
    int err = sem_init(& semaphore, 0, initial);
    ASSERT(err == 0);
}

LinuxSemaphore::~LinuxSemaphore()
{
    int err = sem_destroy(& semaphore);
    ASSERT(err == 0);
}

void LinuxSemaphore::post()
{
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

void LinuxSemaphore::wait_timeout(int ticks)
{
    // TODO
    IGNORE(ticks);
    ASSERT(0);
}

namespace panglos {

Semaphore* Semaphore::create(Type type, int n, int initial)
{
    IGNORE(type);
    IGNORE(n);
    ASSERT(initial >= 0);
    return new LinuxSemaphore(unsigned(initial));
}

}

//  FIN
