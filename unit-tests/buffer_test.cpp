
#include <stdlib.h>
#include <stdarg.h>

#include <gtest/gtest.h>

#include <panglos/debug.h>
#include <panglos/buffer.h>

#include "mock.h"

using namespace panglos;

TEST(RingBuffer, EmptyTest)
{
    RingBuffer buffer(128);

    EXPECT_TRUE(buffer.empty());
    EXPECT_FALSE(buffer.full());

    EXPECT_EQ(-1, buffer.getc());

    uint8_t b[10];
    EXPECT_EQ(0, buffer.gets(b, sizeof(b)));
}

TEST(RingBuffer, Add)
{
    RingBuffer buffer(8);

    EXPECT_TRUE(buffer.empty());

    int n = buffer.add('x');
    EXPECT_EQ(1, n);

    // 7 more will overflow the buffer, will only add 6
    n = buffer.add((const uint8_t*) "abcdefg", 7);
    EXPECT_EQ(6, n);
    EXPECT_TRUE(buffer.full());

    // still full, so add should return 0
    n = buffer.add('z');
    EXPECT_EQ(0, n);

    n = buffer.getc();
    EXPECT_EQ('x', n);
    EXPECT_FALSE(buffer.full());

    n = buffer.getc();
    EXPECT_EQ('a', n);
    n = buffer.getc();
    EXPECT_EQ('b', n);
    n = buffer.getc();
    EXPECT_EQ('c', n);
    n = buffer.getc();
    EXPECT_EQ('d', n);
    n = buffer.getc();
    EXPECT_EQ('e', n);
    n = buffer.getc();
    EXPECT_EQ('f', n);

    EXPECT_TRUE(buffer.empty());

    n = buffer.add('y');
    EXPECT_EQ(1, n);
    EXPECT_FALSE(buffer.empty());

    n = buffer.getc();
    EXPECT_EQ('y', n);
    EXPECT_TRUE(buffer.empty());
}

    /*
     *
     */

typedef struct
{
    EventQueue *q;
    Semaphore *s;
    RingBuffer *b;
    const char *tx;
    bool dead;
    bool running;
    bool done;
    bool kill;
}   Info;

static void * get_fn(void *arg)
{
    ASSERT(arg);
    Info *info = (Info*) arg;

    info->running = true;

    while (!info->dead)
    {
        bool done = info->b->wait(info->q, info->s, 100);
        if (!done)
        {
            continue;
        }

        uint8_t buff[9];
        int c = info->b->gets(buff, sizeof(buff)-1);
        buff[c] = '\0';
        EXPECT_STREQ(info->tx, (char*) buff);
    }

    return 0;
}

static void * add_fn(void *arg)
{
    ASSERT(arg);
    Info *info = (Info*) arg;

    while (!info->running)
    {
        usleep(100);
    }

    usleep(1000);

    for (int i = 0; i < 100; i++)
    {
        int len = strlen(info->tx);
        info->b->add((uint8_t*) info->tx, len);
        usleep(500);
    }

    info->done = true;
 
    return 0;
}

static void * event_fn(void *arg)
{
    ASSERT(arg);
    Info *info = (Info*) arg;

    panglos::timer_t now = 0;

    while (!info->kill)
    {
        usleep(10);
        info->q->check();

        now += 50;
        mock_timer_set(now);
    }

    return 0;
}

TEST(RingBuffer, Wait)
{
    mock_setup(false);

    EventQueue q(0);
    Semaphore *s = Semaphore::create();
    RingBuffer buffer(1024);
    Info info = {
        .q = & q,
        .s = s,
        .b = & buffer,
        .tx = "abcdefgh",
        .dead = false,
        .running = false,
        .done = false,
        .kill = false
    };

    pthread_t add, get, events;
    int err;

    err = pthread_create(& events, 0, event_fn, & info);
    ASSERT(err == 0);

    // this should time out, no data in buffer
    bool done = buffer.wait(& q, s, 100);
    EXPECT_FALSE(done);

    err = pthread_create(& get, 0, add_fn, & info);
    ASSERT(err == 0);

    err = pthread_create(& add, 0, get_fn, & info);
    ASSERT(err == 0);

    while (!info.done)
    {
        usleep(100000);
    }

    info.dead = true;
    pthread_join(get, 0);
    pthread_join(add, 0);
    info.kill = true;
    pthread_join(events, 0);

    delete s;
    mock_teardown();
}

TEST(RingBuffer, Timeout)
{
    mock_setup(false);

    EventQueue q(0);
    Semaphore *s = Semaphore::create();
    RingBuffer buffer(1024);
    Info info = {
        .q = & q,
        .s = s,
        .b = & buffer,
        .tx = "abcdefgh",
        .dead = false,
        .running = false,
        .done = false,
        .kill = false
    };

    pthread_t events;
    int err;

    err = pthread_create(& events, 0, event_fn, & info);
    ASSERT(err == 0);

    // this should time out, no data in buffer
    bool done = buffer.wait(& q, s, 1000);
    EXPECT_FALSE(done);

    info.kill = true;
    pthread_join(events, 0);

    delete s;
    mock_teardown();
}

//  FIN
