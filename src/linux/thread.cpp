
#include <stddef.h>
#include <stdlib.h>
#include <pthread.h>

#include "panglos/debug.h"

#include "panglos/list.h"
#include "panglos/thread.h"

using namespace panglos;

    /*
     *
     */

#if defined(ARCH_LINUX)

class NativeThread;

static __thread NativeThread *native_thread = 0;

static Mutex *mutex = 0;

static void del_mutex()
{
    delete mutex;
}

class NativeThread : public Thread
{
    const char *name;
    pthread_t thread;
    void (*fn)(void*);
    void *arg;

    Thread *next;
public:
    static Thread **get_next(Thread *t) { return & ((NativeThread*)t)->next; }
    typedef List<Thread *> Threads;

    static Threads threads;

public:
    NativeThread(const char *_name)
    :   name(_name),
        thread(0),
        fn(0),
        arg(0),
        next(0)
    {
        if (!mutex)
        {
            mutex = Mutex::create();
            atexit(del_mutex);
        }
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
    
        threads.push(nt, mutex);
        nt->fn(nt->arg);
        threads.remove(nt, mutex);

        pthread_exit(0);
        return 0;
    }

    virtual void start(void (*_fn)(void *arg), void *_arg, int) override
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

NativeThread::Threads NativeThread::threads(NativeThread::get_next);

namespace panglos {

void Thread::visit(int (*fn)(Thread *, void *), void *arg)
{
    NativeThread::threads.visit(fn, arg, mutex);
}

Thread *Thread::create(const char *name, size_t stack, Priority priority)
{
    IGNORE(stack);
    IGNORE(priority);
    return new NativeThread(name);
}

class MainThread : public Thread
{
    virtual void start(void (*fn)(void *arg), void *arg, int) override
    {
        UNUSED(fn);
        UNUSED(arg);
        ASSERT(0);
    }
    virtual void join() override
    {
        ASSERT(0);
    }
    virtual const char *get_name() override
    {
        return "main";
    }
};

Thread *Thread::get_current()
{
    static MainThread main_thread;
    if (native_thread)
    {
        return native_thread;
    }
    return & main_thread;
}

}   //  namespace panglos

#endif //   defined(ARCH_LINUX)

//  FIN
