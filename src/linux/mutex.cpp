
#include <pthread.h>

#include <panglos/debug.h>
#include <panglos/mutex.h>

using namespace panglos;

class NativeMutex : public Mutex
{
    pthread_mutex_t mutex;

public:
    NativeMutex()
    {
        int err = pthread_mutex_init(& mutex, 0);
        ASSERT(err == 0);
    }

private:
    virtual ~NativeMutex()
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

namespace panglos {

Mutex* Mutex::create(Mutex::Type type)
{
    IGNORE(type);
    return new NativeMutex();
}

}   //  namespace panglos

//  FIN
