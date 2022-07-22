
#include <atomic>
#include <string.h>

#include "panglos/debug.h"

#include "panglos/queue.h"
#include "panglos/time.h"
#include "panglos/semaphore.h"
#include "panglos/mutex.h"

using namespace panglos;

    /*
     *
     */

class Linux_Queue : public Queue
{
    Semaphore *sem_get_wait;
    Semaphore *sem_put_wait;
    int raw_size;
    int size;
    int num;
    uint8_t *data;
    std::atomic<int> in;
    std::atomic<int> out;
    std::atomic<int> count;
    Mutex *mutex;
public:
    Linux_Queue(int _size, int _num, Mutex *m)
    :   sem_get_wait(0),
        sem_put_wait(0),
        raw_size(_size),
        size(round(_size)),
        num(_num),
        data(0),
        in(0),
        out(0),
        count(0),
        mutex(m)
    {
        ASSERT(size);
        ASSERT(num);

        sem_get_wait = Semaphore::create();
        sem_put_wait  = Semaphore::create();

        // post() size-1 times, so we can wait on a full queue
        for (int i = 0; i < (size-1); i++)
        {
            sem_put_wait->post();
        }

        data = new uint8_t [size * num];
    }

    ~Linux_Queue()
    {
        delete sem_get_wait;
        delete sem_put_wait;
        delete[] data;
    }

    Message *get_data(const char *text, int idx)
    {
        ASSERT((idx >= 0) && (idx < num));
        uint8_t *p = & data[idx * size];
        //PO_DEBUG("%s idx=%d p=%p", text, idx, p);
        IGNORE(text);
        return (Message *) p;    
    }

    void copy(Message *dst, Message *src)
    {
        ASSERT(dst);
        ASSERT(src);
        //PO_DEBUG("dst=%p src=%p size=%d", dst, src, raw_size);
        memcpy(dst, src, size_t(raw_size));
    }

    virtual bool get(Message *msg, int timeout) override
    {
        ASSERT(msg);
        IGNORE(timeout);
        // TODO : wait on sempahore using sem_timedwait() ~
        sem_get_wait->wait();

        Lock lock(mutex);

        if (in == out)
        {
            // Queue empty
            return false;
        }

        const int next = (out + 1) % num;

        copy(msg, get_data(__FUNCTION__, out));
        out = next;
        count -= 1;

        sem_put_wait->post();

        return true;
    }

    virtual bool put(Message *msg) override
    {
        while (true)
        {
            {
                Lock lock(mutex);

                const int next = (in + 1) % num;

                if (next != out)
                {
                    copy(get_data(__FUNCTION__, in), msg);
                    in = next;
                    count += 1;

                    sem_get_wait->post();    
                    return true;
                }
            }
            // Queue is full, so wait on the next get()
            sem_put_wait->wait();
        }
    }

    virtual int queued() override
    {
        return count;
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
