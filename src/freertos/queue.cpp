
extern "C" {
    #include <freertos/FreeRTOS.h>
    #include <freertos/queue.h>
}

#include "panglos/debug.h"

#include "panglos/arch.h"

#include "panglos/freertos/queue.h"
#include "panglos/queue.h"

namespace panglos {

class RTOS_Queue : public Queue
{
    QueueHandle_t handle;
    bool delete_me;

public:
    RTOS_Queue(int size, int num)
    :   delete_me(true)
    {
        handle = xQueueCreate(num, size);
        ASSERT(handle);
    }
    RTOS_Queue(QueueHandle_t _handle)
    :   delete_me(false)
    {
        handle = _handle;
    }

    ~RTOS_Queue()
    {
        if (delete_me)
        {
            vQueueDelete(handle);
        }
    }

    virtual bool get(Message *msg, int timeout) override
    {
        if (arch_in_irq())
        {
            ASSERT(0);
            return 0;
        }

        BaseType_t ok = xQueueReceive(handle, msg, timeout);
        return ok == pdTRUE;
    }

    virtual bool put(Message *msg) override
    {
        BaseType_t ok;

        if (arch_in_irq())
        {
            BaseType_t wake = pdFALSE;
            ok = xQueueSendFromISR(handle, msg, & wake);
            if (wake)
            {
                portYIELD_FROM_ISR();
            }
        }
        else
        {
            ok = xQueueSend(handle, msg, portMAX_DELAY);
        }
        return ok == pdTRUE;
    }

    virtual int queued()
    {
        return uxQueueMessagesWaiting(handle);
    }
};

Queue *Queue::create(int size, int num)
{
    return new RTOS_Queue(size, num);
}

Queue *queue_wrap(QueueHandle_t handle)
{
    return new RTOS_Queue(handle);
}

}   //  namespace panglos

//  FIN
