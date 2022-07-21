
#include <gtest/gtest.h>

#include "panglos/debug.h"
#include "panglos/queue.h"

//#include "event.h"

using namespace panglos;

TEST(Queue, Leak)
{
    Queue *queue = Queue::create(100, 10, 0);

    delete queue;
}

TEST(Queue, Push)
{
    struct Event {
        int num;
    };
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

//  FIN
