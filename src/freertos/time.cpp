
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "panglos/time.h"

namespace panglos {

void Time::sleep(int secs)
{
    vTaskDelay(configTICK_RATE_HZ * secs);
}

void Time::msleep(int msecs)
{
    vTaskDelay((configTICK_RATE_HZ * msecs) / 1000);
}

Time::tick_t Time::get()
{
    // Warning : cannot be used within an irq
    // TODO : ASSERT(!irq_active())
    return xTaskGetTickCount();
}

}   //  panglos

// FIN
