
#include "debug.h"

#include "deque.h"

namespace panglos {

void deque_init(Deque *deque)
{
    ASSERT(deque);
    deque->head = deque->tail = 0;
}

void deque_push_head(Deque *deque, pList w, pnext next_fn, Mutex *mutex)
{
    Lock lock(mutex);

    // list empty case ..
    if (deque->tail == 0)
    {
        deque->tail = w;
    }

    pList *next = next_fn(w);
    *next = deque->head;
    deque->head = w;
}

void deque_push_tail(Deque *deque, pList w, pnext next_fn, Mutex *mutex)
{
    Lock lock(mutex);

    // tail item has no next item
    * next_fn(w) = 0;

    // list empty case
    if (deque->head == 0)
    {
        deque->head = w;
        deque->tail = w;
        return;
    }

    // point old last-in-list to the new entry
    pList prev = deque->tail;
    * next_fn(prev) = w;

    deque->tail = w;
}

pList deque_pop_head(Deque *deque, pnext next_fn, Mutex *mutex)
{
    Lock lock(mutex);

    pList item = deque->head;

    if (!item)
    {
        // empty list
        return 0;
    }

    if (deque->tail == item)
    {
        // last item in deque
        deque->head = deque->tail = 0;
        return item;
    }

    pList *next = next_fn(item);
    deque->head = *next;
    return item;
}

int deque_size(Deque *deque, pnext next_fn, Mutex *mutex)
{
    Lock lock(mutex);

    int count = 0;
    for (pList *next = & deque->head; *next; next = next_fn(*next))
    {
        count += 1;
    }
    return count;
}

//void list_append(Deque *head, pList w, pnext next_fn, Mutex *mutex);
}   //  namespace panglos

//  FIN
