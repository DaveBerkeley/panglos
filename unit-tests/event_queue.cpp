
#include <gtest/gtest.h>

#include <panglos/debug.h>

#include <panglos/time.h>

#include <panglos/event_queue.h>

using namespace panglos;

    /*
     *
     */

class xEQ : public EvQueue::Event
{
    virtual void run(EvQueue *) override
    {
    }
};

TEST(EventQueue, Cmp)
{
    xEQ e1;
    xEQ e2;

    struct Test {
        panglos::Time::tick_t a;
        panglos::Time::tick_t b;
        int result;
    };

    struct Test tests[] = {
        { 0, 10, 10 },
        { 1000, 1010, 10 },
        { 1010, 1000, -10 },
        { 1000, 1000, 0 },
        { 1000, 0x20000000, int(0x20000000 - 1000) },
        { 1000, 0x40000000, int(0x40000000 - 1000) },
        { 1000, 0x60000000, int(0x60000000 - 1000) },
        { 0x7ffffffe, 0x80000001, 3 },
        { 0x8ffffffe, 0x8fffffff, 1 },
        { 0xf1000000, 0xf1000001, 1 },
        { 0xf1000001, 0xf1000000, -1 },
        { 0xfffffffe, 0xffffffff, 1 },
        { 0xfffffffe, 0, 2 },
        { 0xffffffff, 1, 2 },
    };

    for (size_t i = 0; i < (sizeof(tests) / sizeof(tests[0])); i++)
    {
        struct Test *test = & tests[i];
        e1.when = test->a;
        e2.when = test->b;
        int v = EvQueue::Event::cmp(& e1, & e2);
        EXPECT_EQ(v, test->result);
    }
}

    /*
     *
     */

class TestEvent : public EvQueue::Event
{
    virtual void run(EvQueue *) override
    {
        //PO_DEBUG("%d", when);
    }
public:
    TestEvent(panglos::Time::tick_t t=0)
    :   EvQueue::Event(t)
    {
    }
};

static int qsum(EvQueue::Event *ev)
{
    int sum = 0;
    for (; ev; ev = ev->next)
    {
        sum += ev->when;
    }
    return sum;
}

    /*
     *
     */

TEST(EventQueue, AddDel)
{
    EvQueue queue;

    TestEvent ev1(100);
    TestEvent ev2(200);
    TestEvent ev3(50);

    queue.add(& ev1);
    EXPECT_EQ(100, qsum(queue.events.head));
    EXPECT_EQ(& ev1, queue.events.head);

    queue.add(& ev2);
    EXPECT_EQ(300, qsum(queue.events.head));
    EXPECT_EQ(& ev1, queue.events.head);

    queue.add(& ev3);
    EXPECT_EQ(350, qsum(queue.events.head));
    EXPECT_EQ(& ev3, queue.events.head);

    queue.del(& ev1);
    EXPECT_EQ(250, qsum(queue.events.head));
    EXPECT_EQ(& ev3, queue.events.head);

    queue.del(& ev2);
    EXPECT_EQ(50, qsum(queue.events.head));
    EXPECT_EQ(& ev3, queue.events.head);

    queue.del(& ev3);
    EXPECT_EQ(0, qsum(queue.events.head));
    EXPECT_EQ(0, queue.events.head);
}

TEST(EventQueue, Reschedule)
{
    EvQueue queue;

    TestEvent ev1(100);
    TestEvent ev2(200);
    TestEvent ev3(50);

    queue.add(& ev1);
    EXPECT_EQ(100, qsum(queue.events.head));
    EXPECT_EQ(& ev1, queue.events.head);

    queue.add(& ev2);
    EXPECT_EQ(300, qsum(queue.events.head));
    EXPECT_EQ(& ev1, queue.events.head);

    queue.add(& ev3);
    EXPECT_EQ(350, qsum(queue.events.head));
    EXPECT_EQ(& ev3, queue.events.head);

    queue.reschedule(& ev3, 150);
    EXPECT_EQ(450, qsum(queue.events.head));
    EXPECT_EQ(& ev1, queue.events.head);

    queue.del(& ev1);
    EXPECT_EQ(350, qsum(queue.events.head));
    EXPECT_EQ(& ev3, queue.events.head);

    queue.reschedule(& ev3, 10);
    EXPECT_EQ(210, qsum(queue.events.head));
    EXPECT_EQ(& ev3, queue.events.head);

    queue.reschedule(& ev2, 11);
    EXPECT_EQ(21, qsum(queue.events.head));
    EXPECT_EQ(& ev3, queue.events.head);
}

TEST(EventQueue, Pop)
{
    EvQueue queue;

    TestEvent ev1(100);
    TestEvent ev2(200);
    TestEvent ev3(50);
    TestEvent ev4(50);

    queue.add(& ev1);
    queue.add(& ev2);
    queue.add(& ev3);
    queue.add(& ev4);

    EvQueue::Event *ev;

    ev = queue.pop(0);
    EXPECT_FALSE(ev);
    EXPECT_EQ(400, qsum(queue.events.head));

    ev = queue.pop(49);
    EXPECT_FALSE(ev);
    EXPECT_EQ(400, qsum(queue.events.head));

    ev = queue.pop(50);
    EXPECT_TRUE(ev);
    EXPECT_EQ(350, qsum(queue.events.head));

    ev = queue.pop(49);
    EXPECT_FALSE(ev);
    EXPECT_EQ(350, qsum(queue.events.head));

    ev = queue.pop(100);
    EXPECT_TRUE(ev);
    EXPECT_EQ(300, qsum(queue.events.head));

    ev = queue.pop(100);
    EXPECT_TRUE(ev);
    EXPECT_EQ(200, qsum(queue.events.head));

    ev = queue.pop(100);
    EXPECT_FALSE(ev);
    EXPECT_EQ(200, qsum(queue.events.head));

    ev = queue.pop(199);
    EXPECT_FALSE(ev);
    EXPECT_EQ(200, qsum(queue.events.head));

    ev = queue.pop(201);
    EXPECT_TRUE(ev);
    EXPECT_EQ(0, qsum(queue.events.head));
}

TEST(EventQueue, Run)
{
    EvQueue queue;

    TestEvent ev1(100);
    TestEvent ev2(200);
    TestEvent ev3(50);
    TestEvent ev4(50);

    queue.add(& ev1);
    queue.add(& ev2);
    queue.add(& ev3);
    queue.add(& ev4);

    bool ok;

    ok = queue.run(20);
    EXPECT_FALSE(ok);
    EXPECT_EQ(400, qsum(queue.events.head));

    ok = queue.run(99);
    EXPECT_TRUE(ok);
    EXPECT_EQ(300, qsum(queue.events.head));

    ok = queue.run(201);
    EXPECT_TRUE(ok);
    EXPECT_EQ(0, qsum(queue.events.head));
}

    /*
     *
     */

static void match_queue(EvQueue & queue, panglos::Time::tick_t *ticks, int n)
{
    int i = 0;
    for (EvQueue::Event *ev = queue.events.head; ev; ev = ev->next)
    {
        //PO_DEBUG("%d %d %#x %#x", i, n, ticks[i], ev->when);
        EXPECT_TRUE(i < n);
        EXPECT_EQ(ticks[i], ev->when);
        i += 1;
    }
    EXPECT_TRUE(i == n);
}

#if 0
static void show_queue(EvQueue & queue)
{
    char buff[128] = { 0 };

    for (EvQueue::Event *ev = queue.events.head; ev; ev = ev->next)
    {
        char item[24];
        snprintf(item, sizeof(item), "%#x ", ev->when);
        strcat(buff, item);
    }
    PO_DEBUG("%s", buff);
}
#endif

TEST(EventQueue, Wrap)
{
    EvQueue queue;

    TestEvent ev1(100);
    TestEvent ev2(0xfffffff1);
    TestEvent ev3(200);
    TestEvent ev4(0xfffffff9);
    TestEvent ev5(0);

    queue.add(& ev1);

    {
        panglos::Time::tick_t ticks[] = { 100 };
        match_queue(queue, ticks, 1);
    }

    queue.add(& ev2);

    {
        panglos::Time::tick_t ticks[] = { 0xfffffff1, 100 };
        match_queue(queue, ticks, 2);
    }

    queue.add(& ev3);

    {
        panglos::Time::tick_t ticks[] = { 0xfffffff1, 100, 200 };
        match_queue(queue, ticks, 3);
    }

    queue.add(& ev4);

    {
        panglos::Time::tick_t ticks[] = { 0xfffffff1, 0xfffffff9, 100, 200 };
        match_queue(queue, ticks, 4);
    }

    queue.add(& ev5);

    {
        panglos::Time::tick_t ticks[] = { 0xfffffff1, 0xfffffff9, 0, 100, 200 };
        match_queue(queue, ticks, 5);
    }

    bool ok;

    ok = queue.run(0xfffffff1);
    EXPECT_TRUE(ok);

    {
        panglos::Time::tick_t ticks[] = { 0xfffffff9, 0, 100, 200 };
        match_queue(queue, ticks, 4);
    }

    ok = queue.run(0);
    EXPECT_TRUE(ok);

    {
        panglos::Time::tick_t ticks[] = { 100, 200 };
        match_queue(queue, ticks, 2);
    }

    ok = queue.run(0);
    EXPECT_FALSE(ok);

    ok = queue.run(99);
    EXPECT_FALSE(ok);

    ok = queue.run(150);
    EXPECT_TRUE(ok);
    {
        panglos::Time::tick_t ticks[] = { 200 };
        match_queue(queue, ticks, 1);
    }

    ok = queue.run(1000);
    EXPECT_TRUE(ok);
    {
        panglos::Time::tick_t ticks[] = {  };
        match_queue(queue, ticks, 0);
    }
}

//  FIN
