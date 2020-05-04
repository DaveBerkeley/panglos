
#include <stdlib.h>

#include "debug.h"
#include "mutex.h"

#include "buffer.h"

#define IGNORE(x) ((x) = (x))

namespace panglos {

RingBuffer::RingBuffer(int _size, Semaphore *s, Mutex *_mutex)
: mutex(_mutex), delete_mutex(0), semaphore(s), data(0), in(0), out(0), size(_size)
{
    if (!mutex)
    {
        delete_mutex = mutex = Mutex::create();
    }

    data = (uint8_t*) malloc(_size);
}

RingBuffer::~RingBuffer()
{
    free(data);
    delete delete_mutex;
}

int RingBuffer::_add(uint8_t c, Mutex *m)
{
    Lock lock(m);

    int next = in + 1;
    if (next >= size)
    {
        next = 0;
    }

    if (next == out)
    {
        return 0;
    }

    data[in] = c;
    in = next;

    // post the semaphore, if a task is waiting on us
    panglos::Semaphore *s = semaphore;
    if (s)
    {
        s->post();
    }

    return 1;
}

int RingBuffer::add(uint8_t c)
{
    const int n = _add(c, mutex);

    if (semaphore)
    {
        semaphore->post();
    }

    return n;
}

int RingBuffer::add(const uint8_t *s, int n)
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

bool RingBuffer::empty()
{
    return in == out;
}

bool RingBuffer::full()
{
    Lock lock(mutex);

    int next = in + 1;
    if (next >= size)
    {
        next = 0;
    }

    return next == out;
}

int RingBuffer::_getc()
{
    Lock lock(mutex);

    if (empty())
    {
        // no data
        return -1;
    }

    int next = out + 1;
    if (next >= size)
    {
        next = 0;
    }

    const int c = data[out];
    out = next;

    return c;
}

int RingBuffer::_gets(uint8_t *s, int n)
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

void RingBuffer::reset()
{
    Lock lock(mutex);

    in = out;
}

int RingBuffer::wait(EventQueue *q, timer_t timeout)
{
    if (!empty())
    {
        // no need to wait
        return true;
    }

    ASSERT(semaphore);

    // The semaphore could be triggered by add() or a timeout
    // or possibly both
    q->wait(semaphore, timeout);

    return !empty();
}

}   //  namespace panglos

//  FIN
