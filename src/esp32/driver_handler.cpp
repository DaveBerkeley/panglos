
#include "panglos/debug.h"

#include "panglos/esp32/driver_handler.h"

namespace panglos {

    /*
     *  Handler list
     */

struct DriverHandler::Handler
{
    struct DriverHandler::Handler *next;
    void (*fn)(void *);
    void *arg;
    QueueHandle_t handle;
};

static struct DriverHandler::Handler **next_fn(struct DriverHandler::Handler *item)
{
    return & item->next;
}

static int match(DriverHandler::Handler *item, void *arg)
{
    ASSERT(arg);
    QueueHandle_t handle = (QueueHandle_t) arg;
    return item->handle == handle;
}

    /*
     *
     */

static void dh_run(void *arg)
{
    ASSERT(arg);
    DriverHandler *dh = (DriverHandler*) arg;
    dh->run();
}

    /*
     *
     */

DriverHandler::DriverHandler(int queue_size)
:   set(0),
    thread(0),
    mutex(0),
    handlers(next_fn),
    dead(false)
{
    mutex = Mutex::create();

    set = xQueueCreateSet(queue_size);
    ASSERT(set);
    PO_DEBUG("");

    thread = Thread::create("driver", 1024);
    //thread->start(dh_run, this);
    PO_ERROR("thread->start() needed");
}

DriverHandler::~DriverHandler()
{
    dead = true;
    thread->join();
    delete thread;

    while (handlers.head)
    {
        struct Handler *handler = handlers.pop(mutex);
        delete handler;
    }

    delete mutex;
}

    /*
     *
     */

void DriverHandler::run()
{
    PO_DEBUG("");
    return;

    while (!dead)
    {
        QueueSetMemberHandle_t h = xQueueSelectFromSet(set, 100);
        ASSERT(0);
        if (!h)
        {
            continue;
        }
        PO_DEBUG("%p", h);

        Handler *handler = handlers.find(match, h, mutex);
        if (!handler)
        {
            PO_ERROR("no handler for %p", h);
            continue;
        }

        handler->fn(handler->arg);
    }
}

void DriverHandler::add(Queue *queue, void (*fn)(void *), void *arg)
{
    ASSERT(queue);
    ASSERT(fn);

    struct Handler *handler = new struct Handler;
    handler->next = 0;
    handler->fn = fn;
    handler->arg = arg;
    handler->handle = (QueueHandle_t) queue;

    Lock lock(mutex);

    handlers.push(handler, 0);

    BaseType_t err = xQueueAddToSet((QueueSetMemberHandle_t) queue, set);
    ASSERT(err == pdPASS);
}

void DriverHandler::remove(Queue *queue)
{
    ASSERT(queue);
    Lock lock(mutex);
 
    Handler *handler = handlers.find(match, queue, 0);
    if (!handler)
    {
        PO_ERROR("no handler %p found in list", queue);
        return;
    }

    handlers.remove(handler, 0);
    delete handler;

    BaseType_t err = xQueueRemoveFromSet((QueueSetMemberHandle_t) queue, set);
    ASSERT(err == pdPASS);
}

}   //  namespace panglos

//  FIN
