
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "panglos/debug.h"
#include "panglos/time.h"

namespace panglos {

void Time::sleep(int secs)
{
    vTaskDelay(configTICK_RATE_HZ * secs);
}

void Time::msleep(int msecs)
{
    const Time::tick_t start = Time::get();
    const int ticks = (configTICK_RATE_HZ * msecs) / 1000;

    while (!Time::elapsed(start, ticks))
    {
        // TODO : why doesn't vTaskDelay(n) work?
        vTaskDelay(0);
    }
}

Time::tick_t Time::get()
{
    // Warning : cannot be used within an irq
    // TODO : ASSERT(!irq_active())
    return xTaskGetTickCount();
}

}   //  panglos

// FIN
