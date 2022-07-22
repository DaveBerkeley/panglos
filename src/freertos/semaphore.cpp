
extern "C" {
    #include <freertos/FreeRTOS.h>
    #include <freertos/task.h>
    #include <freertos/semphr.h>
}

#include "panglos/debug.h"
#include "panglos/arch.h"
#include "panglos/semaphore.h"

#include "yield.h"

    /*
     *  Use FreeRTOS Semaphore to implement panglos::Semaphore
     */

class FreeRtosSemaphore : public panglos::Semaphore
{
public:
    SemaphoreHandle_t handle;

    virtual void post() override
    {
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

    virtual void wait_timeout(int ticks) override
    {
        // block until post()
        ASSERT(!arch_in_irq());
        xSemaphoreTake(handle, ticks ? ticks : portMAX_DELAY);
    }

    ~FreeRtosSemaphore()
    {
        vSemaphoreDelete(handle);
    }
};

class Counting : public FreeRtosSemaphore
{
public:
    Counting(int n, int initial)
    {
        handle = xSemaphoreCreateCounting(n, initial);
        ASSERT(handle);
    }
};

class Binary : public FreeRtosSemaphore
{
public:
    Binary()
    {
        handle = xSemaphoreCreateBinary();
        ASSERT(handle);
    }
};

    /*
     *  Factory Functions
     */

namespace panglos {

Semaphore *Semaphore::create(Type type, int n, int initial)
{
    switch (type)
    {
        case NORMAL     : return new Binary;
        case COUNTING   : return new Counting(n, initial);
        default : ASSERT(0);
    }
    return 0;
}

}

//  FIN
