
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "panglos/time.h"

namespace panglos {

void time_sleep(int secs)
{
    vTaskDelay(configTICK_RATE_HZ * secs);
}

void time_msleep(int msecs)
{
    vTaskDelay((configTICK_RATE_HZ * msecs) / 1000);
}

}   //  panglos

// FIN
