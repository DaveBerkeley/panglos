
#if !defined(__DEQUEUE_H__)
#define __DEQUEUE_H__

#include "list.h"

namespace panglos {

typedef struct {
    pList head;
    pList tail;
}   Deque;

void deque_init(Deque *);

void deque_push_head(Deque *deque, pList w, pnext next_fn, Mutex *mutex);
void deque_push_tail(Deque *deque, pList w, pnext next_fn, Mutex *mutex);
int deque_size(Deque *deque, pnext next_fn, Mutex *mutex);
bool deque_empty(Deque *deque);

pList deque_pop_head(Deque *deque, pnext next_fn, Mutex *mutex);

}   //  namespace panglos

#endif  // __DEQUEUE_H__

//  FIN
