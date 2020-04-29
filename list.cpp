
#include "debug.h"
#include "mutex.h"

#include "list.h"

namespace panglos {

static void _list_insert(pList* head, pList w, pList *next)
{
    ASSERT(head);
    ASSERT(w);
    ASSERT(next);

    *next = *head;
    *head = w;
}

    /**
     * @brief return the number of items on the list
     *
     * @param head pointer to the head of the list
     * @param next_fn
     * @param mutex optional
     *
     * @return number of items in the list
     */

int list_size(pList *head, pnext next_fn, Mutex *mutex)
{
    ASSERT(head);
    ASSERT(next_fn);

    int count = 0;

    Lock lock(mutex);

    for (; *head; head = next_fn(*head))
    {
        count += 1;
    }

    return count;
}

/**
 * @brief insert item at the head of the list
 *
 * @param head pointer to the head of the list
 * @param w item to add
 * @param next_fn
 * @param mutex optional
 */

void list_push(pList *head, pList w, pnext next_fn, Mutex *mutex)
{
    ASSERT(head);
    ASSERT(w);
    ASSERT(next_fn);

    Lock lock(mutex);

    _list_insert(head, w, next_fn(w));
}

/**
 * @brief insert item at the end of the list
 *
 * @param head pointer to the head of the list
 * @param w item to add
 * @param next_fn
 * @param mutex optional
 */

void list_append(pList *head, pList w, pnext next_fn, Mutex *mutex)
{
    ASSERT(head);
    ASSERT(w);
    ASSERT(next_fn);

    Lock lock(mutex);

    for (; *head; head = next_fn(*head))
    {
        ;
    }

    _list_insert(head, w, next_fn(w));
}

/**
 * @brief insert item into a sorted list
 *
 * @param head pointer to the head of the list
 * @param w item to add
 * @param next_fn
 * @param cmp compare function
 * @param mutex optional
 */

void list_add_sorted(pList *head, pList w, pnext next_fn, cmp_fn cmp, Mutex *mutex)
{
    ASSERT(head);
    ASSERT(w);
    ASSERT(next_fn);
    ASSERT(cmp);

    Lock lock(mutex);

    for (; *head; head = next_fn(*head))
    {
        if (cmp(w, *head) >= 0)
        {
            break;
        }
    }

    _list_insert(head, w, next_fn(w));
}

/**
 * @brief return the head item of the list
 *
 * @param head pointer to the head of the list
 * @param next_fn
 * @param mutex optional
 */

pList list_pop(pList *head, pnext next_fn, Mutex *mutex)
{
    ASSERT(head);
    ASSERT(next_fn);

    Lock lock(mutex);

    pList w = *head;

    if (w)
    {
        pList* next = next_fn(w);
        *head = *next;
        *next = 0;
    }

    return w;
}

/**
 * @brief remove an item from the list
 *
 * @param head pointer to the head of the list
 * @param w item to remove
 * @param next_fn
 * @param mutex optional
 *
 * @return true if item was removed
 */

bool list_remove(pList *head, pList w, pnext next_fn, Mutex *mutex)
{
    ASSERT(head);
    ASSERT(w);
    ASSERT(next_fn);

    bool found = false;

    Lock lock(mutex);

    for (; *head; head = next_fn(*head))
    {
        if (w == *head)
        {
            // unlink top item
            list_pop(head, next_fn, 0);
            found = true;
            break;
        }
    }

    return found;
}

/**
 * @brief find an item in the list
 *
 * @param head pointer to the head of the list
 * @param next_fn
 * @param fn function used to match each item
 * @param arg arg passed to the match function
 * @param mutex optional
 *
 * @return the found item or null
 */

pList list_find(pList *head, pnext next_fn, visitor fn, void *arg, Mutex *mutex)
{
    ASSERT(head);
    ASSERT(next_fn);
    ASSERT(fn);

    pList item = 0;

    Lock lock(mutex);

    for (; *head; head = next_fn(*head))
    {
        if (fn(*head, arg))
        {
            item = *head;
            break;
        }
    }

    return item;
}

/**
 * @brief visit all items on the list, terminating if fn() returns true
 *
 * @param head pointer to the head of the list
 * @param next_fn
 * @param fn function used to match each item
 * @param arg arg passed to the match function
 * @param mutex optional
 */

void list_visit(pList *head, pnext next_fn, visitor fn, void *arg, Mutex *mutex)
{
    list_find(head, next_fn, fn, arg, mutex);
}

}   //  namespace

//  FIN
