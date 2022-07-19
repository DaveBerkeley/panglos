
extern "C" {
    #include <freertos/FreeRTOS.h>
    #include <freertos/task.h>
}

#include "panglos/debug.h"
#include "panglos/mutex.h"
#include "panglos/semaphore.h"
#include "panglos/thread.h"

namespace panglos {

    /*
     *
     */

class RTOS_Thread;

// Use TLS to save the current thread
// This assumes that TLS is supported by the port of FreeRTOS
// True for esp32
static __thread RTOS_Thread *_this_thread;

class RTOS_Thread : public Thread
{
    const char *name;
    size_t stack;
    Thread::Priority pri;

    void (*fn)(void *arg);
    void *arg;

    TaskHandle_t handle;
    Semaphore *dead;

public:

    RTOS_Thread(const char *_name, size_t _stack, Thread::Priority _pri)
    :   name(_name),
        stack(_stack ? _stack : 4000),
        pri(_pri),
        arg(0),
        handle(0),
        dead(0)
    {
        dead = Semaphore::create();
        _this_thread = this;
    }

    ~RTOS_Thread()
    {
        delete dead;
    }

    virtual void start(void (*fn)(void *arg), void *arg) override;
    virtual void join() override;

    void run()
    {
        ASSERT(fn);
        PO_DEBUG("fn=%p arg=%p", fn, arg);
        fn(arg);
        dead->post();
    }

    virtual const char *get_name() override
    {
        return name;
    }
};

    /*
     *
     */

static void thread_run(void *arg)
{
    RTOS_Thread *thread = (RTOS_Thread *) arg;
    ASSERT(thread);

    PO_DEBUG("%p", thread);

    thread->run();
    vTaskDelete(0);
}

void RTOS_Thread::start(void (*_fn)(void *arg), void *_arg)
{
    PO_DEBUG("%p", this);
    arg = _arg;
    fn = _fn;
    BaseType_t err = xTaskCreate(thread_run, name, stack, this, pri, & handle);
    ASSERT(err == pdPASS);
}

void RTOS_Thread::join()
{
    PO_DEBUG("");
    dead->wait();
}

Thread *Thread::create(const char *name, size_t stack, Thread::Priority pri)
{
    return new RTOS_Thread(name, stack, pri);
}

Thread *Thread::get_current()
{
    return _this_thread;
}

}   //  namespace panglos

//  FIN
