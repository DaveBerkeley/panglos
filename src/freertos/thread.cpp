
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

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

    virtual void start(void (*fn)(void *arg), void *arg, int core) override;
    virtual void join() override;

    void run()
    {
        ASSERT(fn);
        threads.push(this, mutex);
        PO_DEBUG("name=%s fn=%p arg=%p stack=%d", name, fn, arg, stack);
        fn(arg);
        dead->post();
        threads.remove(this, mutex);
    }

    virtual const char *get_name() override
    {
        return name;
    }

    virtual int get_core() override
    {
#if defined(CONFIG_FREERTOS_SMP)
        TaskStatus_t status;
        vTaskGetInfo(0, & status, pdFALSE, eInvalid);
        return xTaskGetCoreID(status.xHandle);
#else
        return 0;
#endif
    }
};

    /*
     *
     */

void Thread::visit(int (*fn)(Thread*, void*), void *arg)
{
    threads.visit((int (*)(RTOS_Thread*, void*)) fn, arg, mutex);
}

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

// TODO : make more widely available ?
static LUT freertos_err[] =
{
    {   "FAIL", pdFAIL },
    {   "COULD_NOT_ALLOCATE_REQUIRED_MEMORY", errCOULD_NOT_ALLOCATE_REQUIRED_MEMORY },
    {   "QUEUE_BLOCKED", errQUEUE_BLOCKED },
    {   "QUEUE_YIELD", errQUEUE_YIELD },
    {   0,  0   },
};

static UBaseType_t priority(Thread::Priority p)
{
    switch (p)
    {
        case Thread::High   :   return 23;
        case Thread::Medium :   return 15;
        case Thread::Low    :   return 0;
        default : break;
    }
    ASSERT(0);
    return -1;
}

void RTOS_Thread::start(void (*_fn)(void *arg), void *_arg, int core)
{
    PO_DEBUG("%p", this);
    arg = _arg;
    fn = _fn;

    // (core > 0) will assert on MPUs with less cores
    while ((core > 0) && !taskVALID_CORE_ID(core))
    {
        PO_WARNING("invalid core id=%d", core);
        core -= 1;
    }

#if defined(CONFIG_FREERTOS_SMP)
    BaseType_t err = xTaskCreatePinnedToCore(thread_run, name, stack, this, priority(pri), & handle, 
            (core == -1) ? tskNO_AFFINITY : core);
#else
    BaseType_t err = xTaskCreate(thread_run, name, stack, this, priority(pri), & handle);
#endif
    if (err != pdPASS)
    {
        PO_ERROR("name=%s stack=%d err=%d '%s'", name, (int) stack, (int) err, lut(freertos_err, err));
        ASSERT(0);
    }
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

    /*
     *  ESP-IDF creates a number of other threads in the background.
     *
     *  We can create a dummy Thread for each of these, 
     *  so the names can be distinguished in the log outout.
     */

static const char *get_task_name()
{
    UBaseType_t num_tasks = uxTaskGetNumberOfTasks();

    if (num_tasks == 0) return "none";

    TaskStatus_t *tasks = (TaskStatus_t*) malloc(sizeof(TaskStatus_t) * num_tasks);

    const char *name = "unknown";

    UBaseType_t n =  uxTaskGetSystemState(tasks, num_tasks, 0);
    ASSERT(n == num_tasks);

    TaskHandle_t handle = xTaskGetCurrentTaskHandle();

    for (UBaseType_t i = 0; i < num_tasks; ++i)
    {
        TaskStatus_t *task = & tasks[i];
        if (task->xHandle == handle)
        {
            name = task->pcTaskName;
            break;
        }
    }

    free(tasks);
    return name;
}


class OtherThread : public Thread
{
public:
    TaskHandle_t handle;
    char name[24];
    class OtherThread *next;

    virtual void start(void (*)(void *), void *, int) override { }
    virtual void join() override { }

    virtual const char *get_name() override
    {
        return name;
    }

    static OtherThread **get_next(OtherThread *t) { return & t->next; }

    static List<OtherThread *> others;

    static int match(OtherThread *t, void *arg)
    {
        ASSERT(arg);
        TaskHandle_t h = (TaskHandle_t) arg;
        return t->handle == h;
    }

    static Thread *find(TaskHandle_t h)
    {
        return others.find(match, h, mutex);
    }

    OtherThread(TaskHandle_t h)
    :   handle(h)
    {
        if (!mutex)
        {
            mutex = Mutex::create();
        }

        snprintf(name, sizeof(name), "%s", get_task_name());
        others.push(this, mutex);
    }
};

List<OtherThread *> OtherThread::others(OtherThread::get_next);

static int match_handle(RTOS_Thread *thread, void *arg)
{
    ASSERT(arg);
    TaskHandle_t handle = (TaskHandle_t) arg;
    return (handle == thread->handle) ? 1 : 0;
}

Thread *Thread::get_current()
{
    TaskHandle_t handle = xTaskGetCurrentTaskHandle();
    Thread *thread = threads.find(match_handle, handle, mutex);
    if (thread)
    {
        return thread;
    }

    // not a Task that we've created as a Thread
    thread = OtherThread::find(handle);

    if (!thread)
    {
        thread = new OtherThread(handle);
    }

    return thread;
}

}   //  namespace panglos

extern "C" {

__attribute__((weak)) uint32_t get_time(void)
{
    TickType_t ticks = xTaskGetTickCount();
    return ticks;
}

const char *get_task(void)
{
    panglos::Thread *thread = panglos::Thread::get_current();
    return thread ? thread->get_name() : pcTaskGetName(0);
}

}

//  FIN
