
#include <stdlib.h>

#include "mutex.h"

#include "buffer.h"

#define IGNORE(x) ((x) = (x))

namespace panglos {

RingBuffer::RingBuffer(int _size)
: mutex(0), semaphore(0), data(0), in(0), out(0), size(_size)
{
    mutex = Mutex::create();
    data = (uint8_t*) malloc(_size);
}

RingBuffer::~RingBuffer()
{
    free(data);
    delete mutex;
}

int RingBuffer::add(uint8_t c, bool use_mutex)
{
    Lock lock(use_mutex ? mutex : 0);

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

int RingBuffer::add(const uint8_t *s, int n, bool use_mutex)
{
    Lock lock(use_mutex ? mutex : 0);

    int count = 0;

    for (int i = 0; i < n; i++)
    {
        if (!add(*s++, false))
        {
            return count;
        }

        count += 1;
    }

    return count;
}

bool RingBuffer::empty(bool use_mutex)
{
    IGNORE(use_mutex); // int reads are atomic
    return in == out;
}

bool RingBuffer::full(bool use_mutex)
{
    Lock lock(use_mutex ? mutex : 0);

    int next = in + 1;
    if (next >= size)
    {
        next = 0;
    }

    return next == out;
}

int RingBuffer::getc(bool use_mutex)
{
    Lock lock(use_mutex ? mutex : 0);

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

int RingBuffer::gets(uint8_t *s, int n, bool use_mutex)
{
    Lock lock(use_mutex ? mutex : 0);

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

int RingBuffer::wait(EventQueue *q, Semaphore *s, timer_t timeout)
{
    if (!empty())
    {
        // no need to wait
        return true;
    }

    // Note : only able to have one semaphore at a time!
    const bool has_sem = semaphore;

    if (!has_sem)
    {
        semaphore = s;
    }

    // The semaphore could be triggered by add() or a timeout
    // or possibly both
    q->wait(s, timeout);

    if (!has_sem)
    {
        // no longer waiting on this semaphore
        semaphore = 0;
    }

    return !empty();
}

}   //  namespace panglos

//  FIN
