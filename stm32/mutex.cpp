
#include <FreeRTOS.h>
#include <semphr.h>

#include <list>

#include <panglos/debug.h>
#include <panglos/mutex.h>
#include <panglos/msg_queue.h>
#include "hal.h"

using namespace panglos;

    /*
     * Use FreeRTOS scheduler suspend / resume to implement panglos::Mutex
     */

class ArmMutex : public Mutex
{
private:

    virtual void lock()
    {
        if (!IS_IN_IRQ())
        {
            vTaskSuspendAll();
        }
    }

    virtual void unlock()
    {
        if (!IS_IN_IRQ())
        {
            xTaskResumeAll();
        }
    }

public:
    ArmMutex()
    {
    }
};


Mutex *Mutex::create()
{
    return new ArmMutex();
}

    /*
     *
     */

class ArmCriticalSection : public Mutex
{
private:

    virtual void lock()
    {
        if (!IS_IN_IRQ())
        {
            taskENTER_CRITICAL();
        }
    }

    virtual void unlock()
    {
        if (!IS_IN_IRQ())
        {
            taskEXIT_CRITICAL();
        }
    }

public:
    ArmCriticalSection()
    {
    }
};

Mutex *Mutex::create_critical_section()
{
    return new ArmCriticalSection();
}

    /*
     *  Use FreeRTOS Semaphore to implement panglos::Semaphore
     */

class ArmSemaphore : public Semaphore
{
    PostHook *hook;
    SemaphoreHandle_t handle;

public:
    virtual void post()
    {
        if (hook)
        {
            hook->post(this);
            return;
        }

        if (IS_IN_IRQ())
        {
            BaseType_t woken = pdFALSE;
            xSemaphoreGiveFromISR(handle, & woken);
            portYIELD_FROM_ISR(woken);
        }
        else
        {
            xSemaphoreGive(handle);
        }
    }

    virtual void wait()
    {
        // block until post()
        ASSERT(!IS_IN_IRQ());
        xSemaphoreTake(handle, portMAX_DELAY);
    }

    virtual void set_hook(PostHook *_hook)
    {
        hook = _hook;
    }

public:
    // use Semaphore::create() to make one

    ArmSemaphore()
    : hook(0)
    {
        handle = xSemaphoreCreateCounting(100, 0);
    }

    ~ArmSemaphore()
    {
        vSemaphoreDelete(handle);
    }
};

Semaphore * Semaphore::create()
{
    return new ArmSemaphore();
}

//  FIN
