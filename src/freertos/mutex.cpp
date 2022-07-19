
extern "C" {
    #include <freertos/FreeRTOS.h>
    #include <freertos/task.h>
    #include <freertos/semphr.h>
}

#include "panglos/debug.h"
#include "panglos/arch.h"
#include "panglos/mutex.h"

#include "yield.h"

    /*
     * Use FreeRTOS scheduler suspend / resume to implement panglos::Mutex
     */

class FreeRtosMutex : public panglos::Mutex
{
private:

    virtual void lock() override
    {
        ASSERT(!arch_in_irq());
        vTaskSuspendAll();
    }

    virtual void unlock() override
    {
        ASSERT(!arch_in_irq());
        xTaskResumeAll();
    }

public:
    FreeRtosMutex()
    {
    }
};

    /*
     *
     */

class FreeRtosRecursive : public panglos::Mutex
{
    SemaphoreHandle_t handle;
private:

    virtual void lock() override
    {
        ASSERT(!arch_in_irq());
        xSemaphoreTakeRecursive(handle, portMAX_DELAY); 
    }

    virtual void unlock() override
    {
        ASSERT(!arch_in_irq());
        xSemaphoreGiveRecursive(handle);
    }

public:
    FreeRtosRecursive()
    {
        handle = xSemaphoreCreateRecursiveMutex();
    }

    ~FreeRtosRecursive()
    {
        vSemaphoreDelete(handle);
    }
};

    /*
     *  Note :- this currently only works with the esp32 freertos port
     */

class FreeRtosCriticalSection : public panglos::Mutex
{
    uint32_t old;
private:

    virtual void lock() override
    {
        old = arch_disable_irq();
    }

    virtual void unlock() override
    {
        arch_restore_irq(old);
    }

public:
    FreeRtosCriticalSection()
    {
    }
};

    /*
     *  Use FreeRTOS Semaphore to implement panglos::Semaphore
     */

class FreeRtosSemaphore : public panglos::Semaphore
{
    panglos::PostHook *hook;
    SemaphoreHandle_t handle;

public:
    virtual void post() override
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
            yield_from_isr();
        }
        else
        {
            xSemaphoreGive(handle);
        }
    }

    virtual void wait() override
    {
        // block until post()
        ASSERT(!arch_in_irq());
        xSemaphoreTake(handle, portMAX_DELAY);
    }

    virtual void set_hook(panglos::PostHook *_hook) override
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

    /*
     *  Factory Functions
     */

namespace panglos {

Mutex *Mutex::create(Mutex::Type type)
{
    switch (type)
    {
        case TASK_LOCK        : return new FreeRtosMutex;
        case CRITICAL_SECTION : return new FreeRtosCriticalSection;
        case RECURSIVE        : return new FreeRtosRecursive;
        default : ASSERT(0);
    }
    return 0;
}

Semaphore * Semaphore::create()
{
    return new FreeRtosSemaphore;
}

}

//  FIN
