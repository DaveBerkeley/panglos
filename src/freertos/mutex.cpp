
extern "C" {
    #include <freertos/FreeRTOS.h>
    #include <freertos/task.h>
    #include <freertos/semphr.h>
}

#include "panglos/debug.h"

#include "panglos/arch.h"

#include "panglos/mutex.h"

using namespace panglos;

    /*
     * Use FreeRTOS scheduler suspend / resume to implement panglos::Mutex
     */

class FreeRtosMutex : public Mutex
{
private:

    virtual void lock()
    {
        if (!arch_in_irq())
        {
            vTaskSuspendAll();
        }
    }

    virtual void unlock()
    {
        if (!arch_in_irq())
        {
            xTaskResumeAll();
        }
    }

public:
    FreeRtosMutex()
    {
    }
};


    /*
     *
     */

#if 0
class FreeRtosCriticalSection : public Mutex
{
private:

    virtual void lock()
    {
        if (!arch_in_irq())
        {
            taskENTER_CRITICAL();
        }
    }

    virtual void unlock()
    {
        if (!arch_in_irq())
        {
            taskEXIT_CRITICAL();
        }
    }

public:
    FreeRtosCriticalSection()
    {
    }
};
#endif

Mutex *Mutex::create(Mutex::Type type)
{
    switch (type)
    {
        case TASK_LOCK          : return new FreeRtosMutex;
        //case CRITICAL_SECTION   : return new FreeRtosCriticalSection;
        case RECURSIVE :
        default : ASSERT(0);
    }
    return 0;
}

    /*
     *  Use FreeRTOS Semaphore to implement panglos::Semaphore
     */

class FreeRtosSemaphore : public Semaphore
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

        if (arch_in_irq())
        {
            BaseType_t woken = pdFALSE;
            xSemaphoreGiveFromISR(handle, & woken);
            portYIELD_FROM_ISR();
        }
        else
        {
            xSemaphoreGive(handle);
        }
    }

    virtual void wait()
    {
        // block until post()
        ASSERT(!arch_in_irq());
        xSemaphoreTake(handle, portMAX_DELAY);
    }

    virtual void set_hook(PostHook *_hook)
    {
        hook = _hook;
    }

public:
    // use Semaphore::create() to make one

    FreeRtosSemaphore()
    : hook(0)
    {
        handle = xSemaphoreCreateCounting(100, 0);
    }

    ~FreeRtosSemaphore()
    {
        vSemaphoreDelete(handle);
    }
};

Semaphore * Semaphore::create()
{
    return new FreeRtosSemaphore();
}

//  FIN
