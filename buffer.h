
#if !defined(__BUFFER_H__)
#define __BUFFER_H__

#include <stdlib.h>

#include "debug.h"
#include "mutex.h"
#include "event.h"

namespace panglos {

template <class T>
class RingBuffer
{
    Mutex *mutex;
    Mutex *delete_mutex;
    Semaphore *semaphore;

    T *data;
    int in, out, size;

    int _add(T c, Mutex *m)
    {
        Lock lock(m);

        int next = in + 1;
        if (next >= size)
        {
            next = 0;
        }

        if (next == out)
        {
            // buffer full
            return 0;
        }

        data[in] = c;
        in = next;

        return 1;
    }

public:

    RingBuffer(int _size, Semaphore *s, Mutex *_mutex=0)
    : mutex(_mutex), delete_mutex(0), semaphore(s), data(0), in(0), out(0), size(_size)
    {
        if (!mutex)
        {
            delete_mutex = mutex = Mutex::create();
        }

        data = (T*) malloc(_size * sizeof(T));
    }

    ~RingBuffer()
    {
        free(data);
        delete delete_mutex;
    }

    int add(T c)
    {
        const int n = _add(c, mutex);

        if (semaphore)
        {
            semaphore->post();
        }

        return n;
    }

    int add(const T *s, int n)
    {
        Lock lock(mutex);
        int count = 0;

        for (int i = 0; i < n; i++)
        {
            if (!_add(*s++, 0))
            {
                return count;
            }

            count += 1;
        }

        if (semaphore)
        {
            semaphore->post();
        }

        return count;
    }

    bool empty()
    {
        return in == out;
    }

    bool full()
    {
        Lock lock(mutex);

        int next = in + 1;
        if (next >= size)
        {
            next = 0;
        }

        return next == out;
    }

    int get(T *s, int n=1)
    {
        Lock lock(mutex);

        for (int i = 0; i < n; i++)
        {
            if (in == out)
            {
                return i;
            }

            int next = out + 1;
            if (next >= size)
            {
                next = 0;
            }

            *s++ = data[out];
            out = next;

        }

        return n;
    }

    void reset()
    {
        Lock lock(mutex);

        in = out;
    }

    int wait(EventQueue *q, timer_t timeout)
    {
        if (!empty())
        {
            // no need to wait
            return true;
        }

        // The semaphore could be triggered by add() or a timeout
        // or possibly both
        q->wait(semaphore, timeout);

        return !empty();
    }

};

}   //  namespace panglos

#endif // __BUFFER_H__

//  FIN
