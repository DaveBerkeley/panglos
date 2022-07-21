
#include <string.h>

#include "panglos/debug.h"

#include "panglos/queue.h"
#include "panglos/semaphore.h"
#include "panglos/mutex.h"

using namespace panglos;

    /*
     *
     */

class Linux_Queue : public Queue
{
    Semaphore *semaphore;
    int raw_size;
    int size;
    int num;
    uint8_t *data;
    int in;
    int out;
    Mutex *mutex;
public:
    Linux_Queue(int _size, int _num, Mutex *m)
    :   semaphore(0),
        raw_size(_size),
        size(round(_size)),
        num(_num),
        data(0),
        in(0),
        out(0),
        mutex(m)
    {
        ASSERT(size);
        ASSERT(num);

        semaphore = Semaphore::create();

        data = new uint8_t [size * num];
    }

    ~Linux_Queue()
    {
        delete semaphore;
        delete[] data;
    }

    Message *get_data(int idx)
    {
        ASSERT((idx >= 0) && (idx < num));
        uint8_t *p = & data[idx * size];
        //PO_DEBUG("idx=%d p=%p", idx, p);
        return (Message *) p;    
    }

    void copy(Message *dst, Message *src)
    {
        //PO_DEBUG("dst=%p src=%p size=%d", dst, src, raw_size);
        memcpy(dst, src, size_t(raw_size));
    }

    virtual bool get(Message *msg, int timeout) override
    {
        ASSERT(msg);
        IGNORE(timeout);
        // TODO : wait on sempahore using sem_timedwait() ~
        semaphore->wait();

        Lock lock(mutex);

        if (in == out)
        {
            // Queue empty
            return false;
        }

        const int next = (out + 1) % num;

        copy(msg, get_data(out));
        out = next;
        return true;
    }

    virtual bool put(Message *msg) override
    {
        Lock lock(mutex);

        const int next = (in + 1) % num;

        if (next == out)
        {
            // Queue Full
            // TODO : should block
            return false;
        }

        copy(get_data(in), msg);
        in = next;

        semaphore->post();
    
        return true;
    }
    virtual int queued() override
    {
        return 0;
    }

    static int round(int n)
    {
        const int size = sizeof(intptr_t);
        n += size - 1;
        return size * (n / size);
    }
};

namespace panglos {

Queue *Queue::create(int size, int num, Mutex *mutex)
{
    return new Linux_Queue(size, num, mutex);
}

}   //  namespace panglos

    /*
     *  Unit tests peculiar to the implmentation.
     */

#include <gtest/gtest.h>

TEST(Queue, Round)
{
    const int size = sizeof(void*);

    EXPECT_EQ(0, Linux_Queue::round(0));
    EXPECT_EQ(size, Linux_Queue::round(size));

    for (int i = 128; i < 200; i++)
    {
        int num = i / size;
        int rem = i % size;
        bool over = rem != 0;

        int expect = (num * size) + (over ? size : 0);

        EXPECT_EQ(expect, Linux_Queue::round(i));
    }
}

//  FIN
