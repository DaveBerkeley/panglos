
#if !defined(__PANGLOS_EVENT_QUEUE__)
#define __PANGLOS_EVENT_QUEUE__

#include <panglos/list.h>
#include <panglos/time.h>
#include <panglos/mutex.h>

namespace panglos {

class EvQueue
{
    Mutex *mutex;
public:
    class Event {
    public:
        Event *next;
        Time::tick_t when;

        Event(Time::tick_t t=0) : next(0), when(t) { }
        virtual ~Event() {}

        virtual void run(EvQueue *) = 0;

        static Event **next_fn(Event *ev) { return & ev->next; }
        static int cmp(Event *ev1, Event* ev2);
    };

    List<Event*> events;

    EvQueue();
    ~EvQueue();

    void add(EvQueue::Event *ev);
    bool del(EvQueue::Event *ev);
    void reschedule(EvQueue::Event *ev, Time::tick_t t);

    EvQueue::Event *pop(Time::tick_t t);

    bool run(Time::tick_t t);
};

}   //  namespace panglos

#endif  //  __PANGLOS_EVENT_QUEUE__

//  FIN
