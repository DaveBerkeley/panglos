
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

Semaphore * Semaphore::create()
{
    return new FreeRtosSemaphore;
}

}

//  FIN
