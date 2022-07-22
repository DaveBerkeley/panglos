
extern "C" {
    #include <freertos/FreeRTOS.h>
    #include <freertos/task.h>
}

#include "panglos/debug.h"
#include "panglos/list.h"
#include "panglos/mutex.h"
#include "panglos/semaphore.h"
#include "panglos/thread.h"

namespace panglos {

    /*
     *
     */

class RTOS_Thread;

// TLS seems to be broken on esp32 + FreeRTOS
//static __thread RTOS_Thread *_this_thread;

class RTOS_Thread;

static RTOS_Thread **get_next(RTOS_Thread*);

static List<RTOS_Thread*> threads(get_next);

static Mutex *mutex = 0;

    /*
     *
     */

class RTOS_Thread : public Thread
{
public:
    RTOS_Thread *next;
    const char *name;
    size_t stack;
    Thread::Priority pri;

    void (*fn)(void *arg);
    void *arg;

    TaskHandle_t handle;
    Semaphore *dead;

public:

    RTOS_Thread(const char *_name, size_t _stack, Thread::Priority _pri)
    :   next(0),
        name(_name),
        stack(_stack ? _stack : 4000),
        pri(_pri),
        arg(0),
        handle(0),
        dead(0)
    {
        dead = Semaphore::create();

        if (!mutex)
        {
            mutex = Mutex::create();
        }
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
        threads.push(this, mutex);
        PO_DEBUG("fn=%p arg=%p", fn, arg);
        fn(arg);
        dead->post();
        threads.remove(this, mutex);
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

static RTOS_Thread **get_next(RTOS_Thread* item)
{
    return & item->next;
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

static int match_handle(RTOS_Thread *thread, void *arg)
{
    ASSERT(arg);
    TaskHandle_t handle = (TaskHandle_t) arg;
    return (handle == thread->handle) ? 1 : 0;
}

Thread *Thread::get_current()
{
    TaskHandle_t handle = xTaskGetCurrentTaskHandle();
    return threads.find(match_handle, handle, mutex);
}

}   //  namespace panglos

//  FIN
