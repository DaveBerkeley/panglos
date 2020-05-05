
#if !defined(__DEQUEUE_H__)
#define __DEQUEUE_H__

#include "list.h"

namespace panglos {

template <class T>
class Deque
{
public:
    T head;
    T tail;

    typedef T* (*pnext)(T item);
 
    pnext next_fn;
public:
    Deque(pnext fn)
    : head(0), tail(0), next_fn(fn)
    {
    }

    void push_head(T w, Mutex *mutex)
    {
        Lock lock(mutex);

        // list empty case ..
        if (tail == 0)
        {
            tail = w;
        }

        T *next = next_fn(w);
        *next = head;
        head = w;
    }

    void push_tail(T w, Mutex *mutex)
    {
        Lock lock(mutex);

        // tail item has no next item
        * next_fn(w) = 0;

        // list empty case
        if (head == 0)
        {
            head = w;
            tail = w;
            return;
        }

        // point old last-in-list to the new entry
        T prev = tail;
        * next_fn(prev) = w;

        tail = w;
    }

    T pop_head(Mutex *mutex)
    {
        Lock lock(mutex);

        T item = head;

        if (!item)
        {
            // empty list
            return 0;
        }

        if (tail == item)
        {
            // last item in deque
            head = tail = 0;
            return item;
        }

        T *next = next_fn(item);
        head = *next;
        return item;
    }

    int size(Mutex *mutex)
    {
        Lock lock(mutex);

        int count = 0;
        for (T *next = & head; *next; next = next_fn(*next))
        {
            count += 1;
        }
        return count;
    }

    bool empty()
    {
        return !head;
    }

};

}   //  namespace panglos

#endif  // __DEQUEUE_H__

//  FIN
