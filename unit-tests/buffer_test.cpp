
#include <stdlib.h>
#include <stdarg.h>

#include <gtest/gtest.h>

#include <panglos/debug.h>
#include <panglos/buffer.h>

#include "mock.h"

using namespace panglos;

TEST(RingBuffer, EmptyTest)
{
    RingBuffer<uint8_t> buffer(128, 0, 0);

    EXPECT_TRUE(buffer.empty());
    EXPECT_FALSE(buffer.full());

    uint8_t data;
    EXPECT_EQ(false, buffer.get(& data));

    uint8_t b[10];
    EXPECT_EQ(0, buffer.get(b, sizeof(b)));
}

TEST(RingBuffer, Add)
{
    RingBuffer<uint8_t> buffer(8, 0, 0);

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

    bool okay;
    uint8_t data;
    okay = buffer.get(& data);
    EXPECT_TRUE(okay);
    EXPECT_EQ('x', data);
    EXPECT_FALSE(buffer.full());

    okay = buffer.get(& data);
    EXPECT_TRUE(okay);
    EXPECT_EQ('a', data);
    okay = buffer.get(& data);
    EXPECT_TRUE(okay);
    EXPECT_EQ('b', data);
    okay = buffer.get(& data);
    EXPECT_TRUE(okay);
    EXPECT_EQ('c', data);
    okay = buffer.get(& data);
    EXPECT_TRUE(okay);
    EXPECT_EQ('d', data);
    okay = buffer.get(& data);
    EXPECT_TRUE(okay);
    EXPECT_EQ('e', data);
    okay = buffer.get(& data);
    EXPECT_TRUE(okay);
    EXPECT_EQ('f', data);

    EXPECT_TRUE(buffer.empty());

    n = buffer.add('y');
    EXPECT_EQ(1, n);
    EXPECT_FALSE(buffer.empty());

    okay = buffer.get(& data);
    EXPECT_TRUE(okay);
    EXPECT_EQ('y', data);
    EXPECT_TRUE(buffer.empty());
}

    /*
     *
     */

typedef struct
{
    EventQueue *q;
    Semaphore *s;
    RingBuffer<uint8_t> *b;
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
        bool done = info->b->wait(info->q, 100);
        if (!done)
        {
            continue;
        }

        uint8_t buff[9];
        int c = info->b->get(buff, sizeof(buff)-1);
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
        int len = (int) strlen(info->tx);
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
    RingBuffer<uint8_t> buffer(1024, s, 0);
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
    bool done = buffer.wait(& q, 100);
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
    RingBuffer<uint8_t> buffer(1024, s, 0);
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
    bool done = buffer.wait(& q, 1000);
    EXPECT_FALSE(done);

    info.kill = true;
    pthread_join(events, 0);

    delete s;
    mock_teardown();
}

    /*
     *
     */

TEST(RingBuffer, Remain)
{
    Semaphore *s = Semaphore::create();
    RingBuffer<uint8_t> buffer(128, s);

    int n;
    uint8_t *start;

    // can't return the whole buffer
    n = buffer.remain(& start);
    EXPECT_EQ(127, n);
    const uint8_t *begin = start;

    // still N-1
    buffer.add('x');
    n = buffer.remain(& start);
    EXPECT_EQ(127, n);

    // total of 64 chars added
    for (int i = 0; i < 63; i++)
    {
        buffer.add('x');    
    }

    // half the buffer should be gone
    n = buffer.remain(& start);
    EXPECT_EQ(64, n);
    EXPECT_EQ(& begin[64], start);

    // add another 62. Should be only 2 chars free
    for (int i = 0; i < 62; i++)
    {
        buffer.add('x');    
    }

    n = buffer.remain(& start);
    EXPECT_EQ(2, n);
    EXPECT_EQ(& begin[126], start);

    // one char free
    buffer.add('x');    
    n = buffer.remain(& start);
    EXPECT_EQ(1, n);
    EXPECT_EQ(& begin[127], start);

    // read half the chars. ie. advance the out pointer
    for (int i = 0; i < 64; i++)
    {
        uint8_t c;
        buffer.get(& c);
    }
 
    // still one char free
    n = buffer.remain(& start);
    EXPECT_EQ(1, n);
    EXPECT_EQ(& begin[127], start);

    // wrap the buff
    buffer.add('x');    
    
    n = buffer.remain(& start);
    EXPECT_EQ(64, n);
    EXPECT_EQ(& begin[0], start);

    for (int i = 0; i < 32; i++)
    {
        buffer.add('x');    
    }

    n = buffer.remain(& start);
    EXPECT_EQ(32, n);
    EXPECT_EQ(& begin[32], start);

    for (int i = 0; i < 64; i++)
    {
        uint8_t c;
        buffer.get(& c);
    }
 
    n = buffer.remain(& start);
    EXPECT_EQ(96, n);
    EXPECT_EQ(& begin[32], start);

    delete s;
}

    /*
     *
     */

TEST(Buffer, Test)
{
    Buffer b(8);
    EXPECT_EQ(0, b.count());

    bool okay;
    okay = b.add('a');
    EXPECT_TRUE(okay);
    EXPECT_EQ(1, b.count());
    okay = b.add('b');
    EXPECT_TRUE(okay);
    EXPECT_EQ(2, b.count());
    okay = b.add('c');
    EXPECT_TRUE(okay);
    EXPECT_EQ(3, b.count());
    EXPECT_FALSE(b.full());

    // read part of the buffer
    uint8_t buff[64];
    int n = b.read(buff, 4);
    EXPECT_EQ(3, n);
    EXPECT_EQ(0, b.count());
    EXPECT_EQ('a', buff[0]);
    EXPECT_EQ('b', buff[1]);
    EXPECT_EQ('c', buff[2]);
    EXPECT_FALSE(b.spent());

    okay = b.add('a');
    EXPECT_TRUE(okay);
    EXPECT_EQ(1, b.count());
    okay = b.add('b');
    EXPECT_TRUE(okay);
    EXPECT_EQ(2, b.count());
    okay = b.add('c');
    EXPECT_TRUE(okay);
    EXPECT_EQ(3, b.count());
    EXPECT_FALSE(b.full());

    // read all requested data
    n = b.read(buff, 2);
    EXPECT_EQ(2, n);
    EXPECT_EQ(1, b.count());
    EXPECT_EQ('a', buff[0]);
    EXPECT_EQ('b', buff[1]);
    EXPECT_FALSE(b.spent());

    // read the rest
    n = b.read(buff, 1);
    EXPECT_EQ(1, n);
    EXPECT_EQ(0, b.count());
    EXPECT_EQ('c', buff[0]);
    EXPECT_FALSE(b.spent());

    // try to read empty buffer
    n = b.read(buff, 1);
    EXPECT_EQ(0, n);
    EXPECT_EQ(0, b.count());
    EXPECT_FALSE(b.spent());

    // buffer should fill up
    okay = b.add('x');
    EXPECT_TRUE(okay);
    EXPECT_FALSE(b.full());
    okay = b.add('y');
    EXPECT_TRUE(okay);
    EXPECT_TRUE(b.full());
    okay = b.add('z');
    EXPECT_FALSE(okay);
    EXPECT_TRUE(b.full());

    EXPECT_FALSE(b.spent());

    // only 2 more available
    n = b.read(buff, 4);
    EXPECT_EQ(2, n);
    EXPECT_EQ('x', buff[0]);
    EXPECT_EQ('y', buff[1]);
    EXPECT_TRUE(b.spent());
}

    /*
     *
     */

TEST(Buffer, Write)
{
    {
        Buffer b(6);

        int n;
        n = b.write((uint8_t*) "abcd", 4);
        EXPECT_EQ(4, n);

        n = b.write((uint8_t*) "efgh", 4);
        EXPECT_EQ(2, n);

        uint8_t buff[16];
        n = b.read(buff, sizeof(buff));
        EXPECT_EQ(6, n);
        buff[n] = '\0';
        EXPECT_STREQ((const char*) "abcdef", (char*) buff);
    }
    {
        Buffer b(6);
        int n;
        n = b.write((uint8_t*) "abcdef", 6);
        EXPECT_EQ(6, n);

        uint8_t buff[16];
        n = b.read(buff, sizeof(buff));
        EXPECT_EQ(6, n);
        buff[n] = '\0';
        EXPECT_STREQ((const char*) "abcdef", (char*) buff);
    }
}

    /*
     *
     */

TEST(Buffers, Test)
{
    Buffers b;

    bool okay;

    okay = b.add('a');
    EXPECT_FALSE(okay);

    b.add_buffer(4);

    // can only add 4 chars
    okay = b.add('a');
    EXPECT_TRUE(okay);
    okay = b.add('b');
    EXPECT_TRUE(okay);
    okay = b.add('c');
    EXPECT_TRUE(okay);
    EXPECT_FALSE(b.full());
    okay = b.add('d');
    EXPECT_TRUE(okay);
    EXPECT_TRUE(b.full());
    okay = b.add('e');
    EXPECT_FALSE(okay);
    EXPECT_TRUE(b.full());

    // b1 = 'abcd'

    b.add_buffer(4);
    EXPECT_FALSE(b.full());
    okay = b.add('e');
    EXPECT_TRUE(okay);
    okay = b.add('f');
    EXPECT_TRUE(okay);
    okay = b.add('g');
    EXPECT_TRUE(okay);
    EXPECT_FALSE(b.full());
    okay = b.add('h');
    EXPECT_TRUE(okay);
    EXPECT_TRUE(b.full());
    okay = b.add('i');
    EXPECT_FALSE(okay);
    EXPECT_TRUE(b.full());

    // b1 = 'abcd'
    // b2 = 'efgh'

    uint8_t buff[64];
    int n;
    n = b.read(buff, 2);
    EXPECT_EQ(2, n);
    EXPECT_EQ('a', buff[0]);
    EXPECT_EQ('b', buff[1]);

    // b1 'cd'
    // b2 'efgh'

    n = b.read(buff, 4);
    EXPECT_EQ(4, n);
    EXPECT_EQ('c', buff[0]);
    EXPECT_EQ('d', buff[1]);
    EXPECT_EQ('e', buff[2]);
    EXPECT_EQ('f', buff[3]);

    // b2 'gh'
    n = b.read(buff, 4);
    EXPECT_EQ(2, n);
    EXPECT_EQ('g', buff[0]);
    EXPECT_EQ('h', buff[1]);
}

TEST(Buffers, Spent)
{
    Buffers b;

    bool okay;

    okay = b.add('a');
    EXPECT_FALSE(okay);

    b.add_buffer(4);

    // add 3 chars
    okay = b.add('a');
    EXPECT_TRUE(okay);
    okay = b.add('b');
    EXPECT_TRUE(okay);
    okay = b.add('c');
    EXPECT_TRUE(okay);
    EXPECT_FALSE(b.full());

    // b1 = 'abc'

    uint8_t buff[64];
    int n;
    n = b.read(buff, 4);
    EXPECT_EQ(3, n);
    EXPECT_EQ('a', buff[0]);
    EXPECT_EQ('b', buff[1]);
    EXPECT_EQ('c', buff[2]);
}

//  FIN
