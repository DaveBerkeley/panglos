
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
     *
     */

class FreeRtosSystem : public panglos::Mutex
{
    SemaphoreHandle_t handle;
private:

    virtual void lock() override
    {
        ASSERT(!arch_in_irq());
        xSemaphoreTake(handle, portMAX_DELAY); 
    }

    virtual void unlock() override
    {
        ASSERT(!arch_in_irq());
        xSemaphoreGive(handle);
    }

public:
    FreeRtosSystem()
    {
        handle = xSemaphoreCreateMutex();
    }

    ~FreeRtosSystem()
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
     *  Factory Functions
     */

namespace panglos {

Mutex *Mutex::create(Mutex::Type type)
{
    switch (type)
    {
        case TASK_LOCK        : return new FreeRtosMutex;
        case CRITICAL_SECTION : return new FreeRtosCriticalSection;
        case SYSTEM           : return new FreeRtosSystem;
        case RECURSIVE        : return new FreeRtosRecursive;
        default : ASSERT(0);
    }
    return 0;
}

}

//  FIN
