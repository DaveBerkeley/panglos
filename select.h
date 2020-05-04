
#if !defined(__SELECT_H__)
#define __SELECT_H__

#include "msg_queue.h"
#include "event.h"

namespace panglos {

    /*
     *
     */

class Select : public PostHook
{
    typedef MsgQueue<Semaphore*> Queue;

    Queue::Deque *deque;
    Mutex *mutex;
    Semaphore *semaphore;
    Queue *queue;
    Semaphore *semaphores;

public:
    Select();
    ~Select();

    virtual void post(Semaphore *s);
    void add(Semaphore *s);
    void remove(Semaphore *s);

    Semaphore *wait();
    Semaphore *wait(EventQueue *eq, timer_t timeout);
};

}   //  namespace

#endif // __SELECT_H__

//  FIN
