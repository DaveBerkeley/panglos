
#include <atomic>

#include <gtest/gtest.h>

#include "panglos/debug.h"
#include "panglos/queue.h"
#include "panglos/mutex.h"
#include "panglos/thread.h"

//#include "event.h"

using namespace panglos;

TEST(Queue, Leak)
{
    Queue *queue = Queue::create(100, 10, 0);

    delete queue;
}

struct Event {
    int num;
};

TEST(Queue, Push)
{
    int num = 10;
    Queue *queue = Queue::create(sizeof(struct Event), num, 0);

    for (int i = 0; i < (num+1); i++)
    {
        struct Event event = { .num = 100 + i, };

        bool ok = queue->put((Queue::Message *) & event);
        EXPECT_TRUE(ok);

        struct Event e;
        ok = queue->get((Queue::Message *) & e, 1);
        EXPECT_TRUE(ok);

        EXPECT_EQ(0, memcmp(& event, & e, sizeof(event)));
    }

    delete queue;
}

TEST(Queue, Queued)
{
    int num = 10;
    Queue *queue = Queue::create(sizeof(struct Event), num, 0);

    EXPECT_EQ(0, queue->queued());

    struct Event event = { .num = 100, };

    bool ok = queue->put((Queue::Message *) & event);
    EXPECT_TRUE(ok);

    EXPECT_EQ(1, queue->queued());
    
    delete queue;
}

    /*
     *
     */

#if 1

struct QT_DEF
{
    Queue *queue;
    int loops;
    std::atomic<int> count;
};

static void qt_test(void *arg)
{
    ASSERT(arg);

    struct QT_DEF *qt = (struct QT_DEF *) arg;

    for (int i = 0; i < qt->loops; i++)
    {
        struct Event event = { .num = qt->count++, };
        bool ok = qt->queue->put((Queue::Message *) & event);
        EXPECT_TRUE(ok);        
    }
}

TEST(Queue, Thread)
{
    const int qsize = 10;
    const int num = 5;
    const int loops = 10000;
    const int total = num * loops;

#if defined(FREERTOS)
    Mutex *mutex = 0;
#else
    Mutex *mutex = Mutex::create();
#endif

    Queue *queue = Queue::create(sizeof(struct Event), qsize, mutex);

    ThreadPool push("thread_%d", num);

    struct QT_DEF qt;
    qt.queue = queue;
    qt.loops = loops;
    qt.count = 0;

    bool found[num*loops];
    memset(found, 0, sizeof(found));

    push.start(qt_test, & qt);

    while ((qt.count < total) || (queue->queued()))
    {
        struct Event event;
        bool ok = queue->get((Queue::Message*) & event, 0);
        EXPECT_TRUE(ok);
        EXPECT_TRUE((event.num >= 0) && (event.num < total));
        found[event.num] = true;
    }

    push.join();

    for (int i = 0; i < total; i++)
    {
        EXPECT_TRUE(found[i]);
    }

    delete queue;
    delete mutex;
}

#endif

//  FIN
