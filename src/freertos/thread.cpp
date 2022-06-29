
extern "C" {
    #include <freertos/FreeRTOS.h>
    #include <freertos/task.h>
}

#include "panglos/debug.h"
#include "panglos/mutex.h"

#include "panglos/thread.h"

namespace panglos {

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
        stack(_stack),
        pri(_pri),
        arg(0),
        handle(0),
        dead(0)
    {
        dead = Semaphore::create();
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
};

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

}   //  namespace panglos

//  FIN
