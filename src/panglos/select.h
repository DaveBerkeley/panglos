
#if !defined(__SELECT_H__)
#define __SELECT_H__

#include "deque.h"
#include "event.h"

namespace panglos {

    /*
     *
     */

class Select : public PostHook
{
    typedef Deque<Semaphore*> Queue;

    Mutex *mutex;
    Semaphore *semaphore;
    Queue queue;
    Semaphore **captured;
    int size;

public:
    Select(int size);
    virtual ~Select();

    virtual void post(Semaphore *s);
    void add(Semaphore *s);
    void remove(Semaphore *s);

    Semaphore *wait();
    Semaphore *wait(EventQueue *eq, timer_t timeout);

    // for test only
    void visit(int (*fn)(Semaphore*, void*), void*);
};

}   //  namespace

#endif // __SELECT_H__

//  FIN
