
#include <stddef.h>
#include <pthread.h>

#include "panglos/debug.h"

#include "panglos/thread.h"

using namespace panglos;

    /*
     *
     */

class NativeThread;

static __thread NativeThread *native_thread = 0;

class NativeThread : public Thread
{
    const char *name;
    pthread_t thread;
    void (*fn)(void*);
    void *arg;
public:
    NativeThread(const char *_name)
    :   name(_name),
        thread(0),
        fn(0)
    {
    }

    virtual ~NativeThread()
    {
    }

    static void *wrap(void *arg)
    {
        ASSERT(arg);
        NativeThread *nt = (NativeThread*) arg;
        native_thread = nt;
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
    }

    virtual void join() override
    {
        int err = pthread_join(thread, 0);
        ASSERT(err == 0);
    }

    virtual const char *get_name() override
    {
        return name;
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
    return native_thread;
}

}   //  namespace panglos

//  FIN
