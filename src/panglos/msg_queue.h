
#if !defined(__PANGLOS_MSG_QUEUE__)
#define __PANGLOS_MSG_QUEUE__

#include <deque>

#include "mutex.h"

namespace panglos {

template <class T>
class MsgQueue 
{
public:
    typedef std::deque<T> Deque;

private:
    /// double ended list of messages
    Deque *q;
    /// mutex used to lock the queue
    Mutex *mutex;
    /// semaphore used to wait on the queue
    Semaphore *semaphore;

public:

    MsgQueue<T>(Deque *_q, Mutex *m, Semaphore *s)
    : q(_q), mutex(m), semaphore(s)
    {
    }

    bool empty()
    {
        Lock lock(mutex);

        return q->empty();
    }

    void put(T t)
    {
        {
            Lock lock(mutex);
            // queue to the front of the list
            q->push_front(t);
        }

        if (semaphore)
        {
            semaphore->post();
        }
    }

    bool _get(T& t)
    {
        Lock lock(mutex);

        if (q->empty())
        {
            t = 0;
            return false;
        }

        // unqueue from the end of the list
        t = q->back();
        q->pop_back();
        return true;
    }

    T get()
    {
        T t;
       
        _get(t);
        return t;
    }

    T wait()
    {
        ASSERT(semaphore);

        // block until data is ready
        while (true)
        {
            T t;
       
            if (_get(t))
            {
                return t;
            }

            // wait for semaphore
            semaphore->wait();
        }
    }
};

}   //  namespace panglos

#endif // __PANGLOS_MSG_QUEUE__

//  FIN
