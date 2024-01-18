
#if !defined(__PANGLOS_EVENT_QUEUE__)
#define __PANGLOS_EVENT_QUEUE__

#include <panglos/list.h>
#include <panglos/time.h>
#include <panglos/mutex.h>

namespace panglos {

template <class T>
class _EvQueue
{
    Mutex *mutex;
public:
    class Event {
    public:
        Event *next;
        T when;

        Event(T t=0) : next(0), when(t) { }
        virtual ~Event() {}

        virtual void run(_EvQueue *) = 0;

        static Event **next_fn(Event *ev) { return & ev->next; }
        static int cmp_t(T t1, T t2);
        static int cmp(Event *ev1, Event* ev2)
        {
            return cmp_t(ev1->when, ev2->when);
        }
    };

    List<Event*> events;

    _EvQueue()
    :   mutex(0),
        events(Event::next_fn)
    {
        mutex = panglos::Mutex::create();
    }

    ~_EvQueue()
    {
        delete mutex;
    }

    void add(_EvQueue::Event *ev)
    {
        events.add_sorted(ev, Event::cmp, mutex);
    }

    bool del(_EvQueue::Event *ev)
    {
        return events.remove(ev, mutex);
    }

    void reschedule(_EvQueue::Event *ev, T t)
    {
        del(ev);
        ev->when = t;
        add(ev);
    }

    Event *pop(T t)
    {
        panglos::Lock lock(mutex);
        _EvQueue::Event *ev = events.head;

        if (!ev)
        {
            return 0;
        }

        int diff = Event::cmp_t(ev->when, t);

        if (diff < 0)
        {
            return 0;
        }

        return events.pop(0);
    }

    bool run(T t)
    {
        bool found = false;

        while (true)
        {
            _EvQueue::Event *ev = pop(t);
            if (!ev)
            {
                return found;
            }
            ev->run(this);
            found = true;
        }
    }
};

typedef _EvQueue<Time::tick_t> EvQueue;

}   //  namespace panglos

#endif  //  __PANGLOS_EVENT_QUEUE__

//  FIN
