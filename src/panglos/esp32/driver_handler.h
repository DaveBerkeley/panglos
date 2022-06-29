
#if !defined(__PANGLOSS_DRIVER_HANDLER__)
#define __PANGLOSS_DRIVER_HANDLER__

extern "C" {
    #include <freertos/FreeRTOS.h>
    #include <freertos/queue.h>
}

#include "panglos/thread.h"
#include "panglos/list.h"

namespace panglos {

class Thread;
class Mutex;

class DriverHandler
{
public:
    struct Handler;
    class Queue;
private:
    QueueSetHandle_t set;
    Thread *thread;
    Mutex *mutex;

    List<struct Handler*> handlers;
    bool dead;

public:
    void add(Queue *queue, void (*fn)(void *), void *arg);
    void remove(Queue *queue);

    DriverHandler(int queue_size); 
    ~DriverHandler();

    void run();
};

}   //  namespace panglos

#endif  //  __PANGLOSS_DRIVER_HANDLER__

//  FIN
