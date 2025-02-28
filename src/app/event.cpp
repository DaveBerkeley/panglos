
#include "panglos/debug.h"
#include "panglos/queue.h"

#include "panglos/list.h"

#include "panglos/app/event.h"

namespace panglos {

    /*
     *
     */

typedef Queue eQueue;

static eQueue *create(int num, Mutex *mutex)
{
    return Queue::create(sizeof(Event), num, mutex);
}
   
Event::Queue *Event::create_queue(int num, Mutex *mutex)
{
    return (Event::Queue*) create(num, mutex);
}

bool Event::put(Queue *queue)
{
    ASSERT(queue);
    eQueue *q = (eQueue*) queue;
    return q->put((eQueue::Message*) this);
}

bool Event::get(Queue *queue, int timeout)
{
    ASSERT(queue);
    eQueue *q = (eQueue*) queue;
    return q->get((eQueue::Message*) this, timeout);
}

int Event::queued(Queue *queue)
{
    eQueue *q = (eQueue*) queue;
    return q->queued();
}

void Event::stop(Queue *queue)
{
    Event event;
    event.type = Event::STOP;
    event.put(queue);
}

    /*
     *
     */

const LUT Event::type_lut[] = {
    {   "KEY", KEY, },
    {   "UART", UART, },
    {   "MQTT", MQTT, },
    {   "DISPLAY", DISPLAY, },
    {   "DATE_TIME", DATE_TIME, },
    {   "INIT", INIT, },
    {   "IDLE", IDLE, },
    {   "LOCATION", LOCATION, },
    {   "STOP", STOP, },
    {   "USER", USER, },
    {   0, 0 },
};

    /*
     *  Modules can register and remove event handlers
     */

typedef List<struct EventHandler*> Handlers;

static Handlers handlers(EventHandler::get_next);

void EventHandler::add_handler(enum Event::Type type, bool (*fn)(void *arg, Event *event, Event::Queue *), void *arg)
{
    struct EventHandler* eh = new struct EventHandler;
    eh->type = type;
    eh->fn = fn;
    eh->arg = arg;
    eh->next = 0;
    PO_DEBUG("type=%s=%d fn=%p eh=%p", lut(Event::type_lut, type), type, fn, eh);
    handlers.push(eh, 0);
}

bool EventHandler::del_handler(EventHandler *eh)
{
    PO_DEBUG("%p", eh);
    const bool ok = handlers.remove(eh, 0);
    delete eh;
    return ok;
}

struct EventHandler *EventHandler::handle_event(Event::Queue *queue, Event *event)
{
    for (struct EventHandler *eh = handlers.head; eh; eh = eh->next)
    {
        if (eh->type == event->type)
        {
            ASSERT(eh->fn);
            const bool ok = eh->fn(eh->arg, event, queue);
            if (ok)
            {
                return eh;
            }
        }
    }

    return 0;
}

static int type_match(struct EventHandler *eh, void *arg)
{
    ASSERT(eh);
    ASSERT(arg);
    Event::Type *type = (Event::Type *) arg;
    return eh->type == *type;
}

void EventHandler::unlink_handlers(Event::Type type)
{
    while (true)
    {
        struct EventHandler *ev = handlers.find(type_match, & type, 0);
        if (!ev)
        {
            break;
        }
        handlers.remove(ev, 0);
        PO_DEBUG("unlink %s handler %p", lut(Event::type_lut, type), ev);
        delete ev;
    }
}

enum Event::Type EventHandler::get_user_code()
{
    static int code = Event::USER;
    return (enum Event::Type) ++code;
}

    /*
     *  UART interrupt event : post UART message
     */

void on_uart(In::Event* ev, void *arg)
{
    // UART interrupt handler
    IGNORE(ev);
    ASSERT(arg);
    struct UartHandler *uh = (struct UartHandler*) arg;
    Event event;
    event.type = Event::UART;
    event.payload.vp.arg = uh->uart;
    event.put(uh->queue);
}

}   //  namespace event

//  FIN
