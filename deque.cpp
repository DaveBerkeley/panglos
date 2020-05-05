
#include "debug.h"

#include "deque.h"

namespace panglos {

Deque::Deque(pnext fn)
: head(0), tail(0), next_fn(fn)
{
}
    
void Deque::push_head(pList w, Mutex *mutex)
{
    Lock lock(mutex);

    // list empty case ..
    if (tail == 0)
    {
        tail = w;
    }

    pList *next = next_fn(w);
    *next = head;
    head = w;
}

void Deque::push_tail(pList w, Mutex *mutex)
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
    pList prev = tail;
    * next_fn(prev) = w;

    tail = w;
}

pList Deque:: pop_head(Mutex *mutex)
{
    Lock lock(mutex);

    pList item = head;

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

    pList *next = next_fn(item);
    head = *next;
    return item;
}

int Deque::size(Mutex *mutex)
{
    Lock lock(mutex);

    int count = 0;
    for (pList *next = & head; *next; next = next_fn(*next))
    {
        count += 1;
    }
    return count;
}

bool Deque::empty()
{
    return !head;
}

}   //  namespace panglos

//  FIN
