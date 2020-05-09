
#if !defined(__BUFFER_H__)
#define __BUFFER_H__

#include <stdlib.h>

#include "debug.h"
#include "mutex.h"
#include "event.h"
#include "deque.h"

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

    /*
     *
     */

class Buffer {
    Buffer *next;
    int size;
    int in;
    int out;
    uint8_t *data;

public:
    Buffer(int _size)
    : next(0), size(_size), in(0), out(0)
    {
        data = (uint8_t*) malloc(size);
    }

    ~Buffer()
    {
        free(data);
    }

    bool add(uint8_t c)
    {
        int next = in + 1;
        if (next > size)
        {
            return false;
        }

        data[in] = c;
        in = next;
        return true;
    }

    int write(const uint8_t *data, int len)
    {
        // TODO : use memcpy
        for (int i = 0; i < len; i++)
        {
            if (!add(*data++))
            {
                return i;
            }
        }
        return len;
    }

    int read(uint8_t *buffer, int n)
    {
        int count = 0;
        for (int i = out; i < in; i++)
        {
            *buffer++ = data[i];
            out += 1;
            count += 1;
            if (count >= n)
            {
                break;
            }
        }
        return count;
    }

    bool full()
    {
        return in == size;
    }

    bool spent()
    {
        return out == size;
    }

    static Buffer **next_fn(Buffer *b)
    {
        return & b->next;
    }
};

    /*
     *
     */

class Buffers
{
    Deque<Buffer*> deque;
    Mutex *mutex;
public:

    Buffers()
    : deque(Buffer::next_fn), mutex(0)
    {
        mutex = Mutex::create();
    }

    ~Buffers()
    {
        while (!deque.empty())
        {
            Buffer *b = deque.pop_head(0);
            delete b;
        }

        delete mutex;
    }

    void add_buffer(int size)
    {
        Buffer *b = new Buffer(size);
        deque.push_tail(b, mutex);
    }

    bool add(uint8_t c)
    {
        Lock lock(mutex);

        Buffer *b = deque.tail;
        if (!b)
        {
            // no buffer allocated
            return false;
        }

        return b->add(c);
    }

    bool full()
    {
        Lock lock(mutex);

        Buffer *b = deque.tail;

        if (!b)
        {
            // no buffer, therefore full
            return true;
        }

        return b->full();        
    }

    int read(uint8_t *buff, int len)
    {
        int count = 0;
        int more = len;

        Lock lock(mutex);

        while (more)
        {
            Buffer *b = deque.head;
            if (!b)
            {
                break;
            }

            const int n = b->read(buff, more);
            if (n == 0)
            {
                break;
            }
            count += n;
            buff += n;
            more -= n;
            if (b->spent())
            {
                deque.pop_head(0);
                delete b;
            }
        }

        return count;
    }
};

}   //  namespace panglos

#endif // __BUFFER_H__

//  FIN
