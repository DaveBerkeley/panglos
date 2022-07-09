
#include <string>
#include <map>

#include <stddef.h>
#include <pthread.h>

#include "panglos/debug.h"
#include "panglos/mutex.h"

#include "panglos/thread.h"

using namespace panglos;

class NativeThread;

typedef std::map<pthread_t, NativeThread*> Map;

static Map map;
static Mutex *mutex = 0;

static void tidy_up()
{
    delete mutex;
    mutex = 0;
}

    /*
     *
     */

class NativeThread : public Thread
{
    std::string name;
    pthread_t thread;
    void (*fn)(void*);
    void *arg;
public:
    NativeThread(const char *_name)
    :   name(_name),
        thread(0),
        fn(0)
    {
        if (!mutex)
        {
            mutex = Mutex::create();
            atexit(tidy_up);
        }
    }

    virtual ~NativeThread()
    {
        Lock lock(mutex);
        map.erase(thread);
    }

    static void *wrap(void *arg)
    {
        ASSERT(arg);
        NativeThread *nt = (NativeThread*) arg;
        ASSERT(nt->fn);
        nt->fn(nt->arg);
        pthread_exit(0);
        return 0;
    }

    virtual void start(void (*_fn)(void *arg), void *_arg) override
    {
        fn = _fn;
        arg = _arg;
        int err = pthread_create(& thread, 0, wrap, this);
        ASSERT(err == 0);

        Lock lock(mutex);
        map[thread] = this;
    }

    virtual void join() override
    {
        int err = pthread_join(thread, 0);
        ASSERT(err == 0);
    }

    virtual const char *get_name() override
    {
        return name.c_str();
    }
};

namespace panglos {

Thread *Thread::create(const char *name, size_t stack, Priority priority)
{
    IGNORE(stack);
    IGNORE(priority);
    return new NativeThread(name);
}

Thread *Thread::get_current()
{
    pthread_t pthread = pthread_self();
    Lock lock(mutex);
    return map[pthread];
}

}   //  namespace panglos

//  FIN
