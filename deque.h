
#if !defined(__DEQUEUE_H__)
#define __DEQUEUE_H__

#include "list.h"

namespace panglos {

class Deque
{
private:
public:
    pList head;
    pList tail;
    pnext next_fn;
public:
    Deque(pnext next_fn);
    
    void push_head(pList w, Mutex *mutex);
    void push_tail(pList w, Mutex *mutex);
    int size(Mutex *mutex);
    bool empty();
    pList pop_head(Mutex *mutex);
};

}   //  namespace panglos

#endif  // __DEQUEUE_H__

//  FIN
