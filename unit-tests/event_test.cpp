
#include <stdlib.h>

#include <gtest/gtest.h>

#include <panglos/debug.h>
#include <panglos/event.h>
#include <panglos/timer.h>

#include "mock.h"

using namespace panglos;

    /*
     *
     */

TEST(Event, AddRemove)
{
    mock_setup(false);

    EventQueue eq(0);
    Semaphore *s = Semaphore::create();

    EXPECT_TRUE(eq.events.empty());

    const int num = 1000;

    // check that the smallest number is at the front
    for (int i = num; i > 0; i--)
    {
        Event *ev = new Event(s, i);

        eq.add(ev);
        EXPECT_EQ(ev, eq.events.head);
    }

    for (int i = 0; i < num; i++)
    {
        Event *ev = eq.events.head;
        Event *e = eq.remove(ev);
        EXPECT_TRUE(e);
        delete e;
    }

    EXPECT_TRUE(eq.events.empty());
    delete s;

    mock_teardown();
}

    /*
     *
     */

TEST(Event, Check)
{
    mock_setup(false);

    EventQueue eq(0);

    // No pending events
    d_timer_t dt = eq.check();
    EXPECT_EQ(0, dt);

    // add some events
    MockSemaphore *ms1 = new MockSemaphore();
    Event *ev1 = new Event(ms1, 10000);
    MockSemaphore *ms2 = new MockSemaphore();
    Event *ev2 = new Event(ms2, 200099);
    MockSemaphore *ms3 = new MockSemaphore();
    Event *ev3 = new Event(ms3, 1000);

    eq.add(ev1);
    eq.add(ev2);
    eq.add(ev3);

    // Should show next event
    dt = eq.check();
    EXPECT_EQ(1000, dt);

    mock_timer_set((panglos::timer_t)995);
    // no event, but nearly ..
    dt = eq.check();
    EXPECT_EQ(5, dt);
    EXPECT_EQ(false, ms1->set);
    EXPECT_EQ(false, ms2->set);
    EXPECT_EQ(false, ms3->set);

    mock_timer_set((panglos::timer_t)1010);
    // one pending event
    dt = eq.check();
    EXPECT_EQ(10000 - 1010, dt);
    EXPECT_EQ(false, ms1->set);
    EXPECT_EQ(false, ms2->set);
    EXPECT_EQ(true, ms3->set);

    mock_timer_set((panglos::timer_t)9999);
    // no event, but nearly
    dt = eq.check();
    EXPECT_EQ(1, dt);
    EXPECT_EQ(false, ms1->set);
    EXPECT_EQ(false, ms2->set);
    EXPECT_EQ(true, ms3->set);

    mock_timer_set((panglos::timer_t)10000);
    // no pending event
    dt = eq.check();
    EXPECT_EQ(200099 - 10000, dt);
    EXPECT_EQ(true, ms1->set);
    EXPECT_EQ(false, ms2->set);
    EXPECT_EQ(true, ms3->set);

    mock_timer_set((panglos::timer_t)200000);
    // no pending event
    dt = eq.check();
    EXPECT_EQ(99, dt);
    EXPECT_EQ(true, ms1->set);
    EXPECT_EQ(false, ms2->set);
    EXPECT_EQ(true, ms3->set);

    mock_timer_set((panglos::timer_t)200100);
    // no pending event
    dt = eq.check();
    EXPECT_EQ(0, dt);
    EXPECT_EQ(true, ms1->set);
    EXPECT_EQ(true, ms2->set);
    EXPECT_EQ(true, ms3->set);

    delete ev1->semaphore;
    delete ev1;
    delete ev2->semaphore;
    delete ev2;
    delete ev3->semaphore;
    delete ev3;

    mock_teardown();
}

    /*
     *
     */

TEST(Event, Wrap)
{
    mock_setup(false);

    EventQueue eq(0);

    // No pending events
    d_timer_t dt = eq.check();
    EXPECT_EQ(0, dt);

    // add some events
    MockSemaphore *ms1 = new MockSemaphore();
    Event *ev1 = new Event(ms1, 0xFFFFFF00);
    eq.add(ev1);
    MockSemaphore *ms2 = new MockSemaphore();
    Event *ev2 = new Event(ms2, 0x00000010);
    eq.add(ev2);
    MockSemaphore *ms3 = new MockSemaphore();
    Event *ev3 = new Event(ms3, 0xFFFFFFFF);
    eq.add(ev3);

    // Check the list order is correct
    Event *ev = eq.events.head;
    EXPECT_EQ(ev->time, 0xFFFFFF00);
    ev = ev->next;
    EXPECT_EQ(ev->time, 0xFFFFFFFF);
    ev = ev->next;
    EXPECT_EQ(ev->time, 0x00000010);
    ev = ev->next;
    EXPECT_EQ(ev, (void*)0);

    // Advance the clock towards the end of the wrap point

    // check the first event triggers
    mock_timer_set((panglos::timer_t)0xFFFFFF00);
    dt = eq.check();
    EXPECT_EQ(0xFFFFFFFF - 0xFFFFFF00, dt);
    EXPECT_EQ(true, ms1->set);
    EXPECT_EQ(false, ms2->set);
    EXPECT_EQ(false, ms3->set);

    mock_timer_set((panglos::timer_t)0xFFFFFFF0);
    dt = eq.check();
    EXPECT_EQ(0xF, dt);
    EXPECT_EQ(true, ms1->set);
    EXPECT_EQ(false, ms2->set);
    EXPECT_EQ(false, ms3->set);

    // wrap the timer. This should trigger one and alert the remaining event
    mock_timer_set((panglos::timer_t)0);
    dt = eq.check();
    EXPECT_EQ(0x10, dt);
    EXPECT_EQ(true, ms1->set);
    EXPECT_EQ(false, ms2->set);
    EXPECT_EQ(true, ms3->set);

    // triger the last event
    mock_timer_set((panglos::timer_t)1000);
    dt = eq.check();
    EXPECT_EQ(0, dt);
    EXPECT_EQ(true, ms1->set);
    EXPECT_EQ(true, ms2->set);
    EXPECT_EQ(true, ms3->set);
    // the events should have all been removed from the queue
    EXPECT_TRUE(eq.events.empty());

    delete ev1->semaphore;
    delete ev1;
    delete ev2->semaphore;
    delete ev2;
    delete ev3->semaphore;
    delete ev3;

    mock_teardown();
}

    /*
     *
     */

TEST(Event, OutOfDate)
{
    mock_setup(false);

    EventQueue eq(0);

    // No pending events
    d_timer_t dt = eq.check();
    EXPECT_EQ(0, dt);

    // move the clock to the future
    mock_timer_set((panglos::timer_t)10000);

    // add historic event
    MockSemaphore *ms1 = new MockSemaphore();
    Event *ev1 = new Event(ms1, 1000);
    eq.add(ev1);

    // should immeadiately run the historic event
    dt = eq.check();
    EXPECT_EQ(0, dt);
    EXPECT_EQ(true, ms1->set);

    delete ev1->semaphore;
    delete ev1;

    mock_teardown();
}

    /*
     *
     */

class TestRescheduler : public Rescheduler
{
public:
    int count;

    TestRescheduler() : count(0) { }

    virtual void reschedule(EventQueue *eq, d_timer_t dt)
    {
        IGNORE(eq);
        IGNORE(dt);
        count += 1;
    }
};

TEST(Event, Reschedule)
{
    mock_setup(false);

    TestRescheduler *r = new TestRescheduler();
    EventQueue eq(r);
 
    // this should reschedule
    MockSemaphore *ms1 = new MockSemaphore();
    Event *ev1 = new Event(ms1, 1000);
    bool first = eq.add(ev1);
    if (first)
    {
        eq.reschedule(1);
    }
    EXPECT_EQ(1, r->count);

    // this won't reschedule
    MockSemaphore *ms2 = new MockSemaphore();
    Event *ev2 = new Event(ms2, 2000);
    first = eq.add(ev2);
    if (first)
    {
        eq.reschedule(1);
    }
    EXPECT_EQ(1, r->count);

    // this should reschedule
    MockSemaphore *ms3 = new MockSemaphore();
    Event *ev3 = new Event(ms3, 500);
    first = eq.add(ev3);
    if (first)
    {
        eq.reschedule(1);
    }
    EXPECT_EQ(2, r->count);

    delete ev1->semaphore;
    delete ev1;
    delete ev2->semaphore;
    delete ev2;
    delete ev3->semaphore;
    delete ev3;

    delete r;

    mock_teardown();
}

    /*
     *
     */

static const int granularity = 10;

typedef struct {
    EventQueue *eq;
    int id;
    int *deltas;
    int count;
    bool dead;
}   Waiter;

static void* wait_fn(void *arg)
{
    Waiter *w = (Waiter*) arg;
    Semaphore *s = Semaphore::create();

    for (int *pi = w->deltas; *pi != -1; pi++)
    {
        const panglos::timer_t now = panglos::timer_now();

        int delay = *pi;
        w->eq->wait(s, delay);
        w->count += 1;

        // check the delay is in the right range of values
        const panglos::timer_t end = panglos::timer_now();
        const int diff = end - now;
        EXPECT_TRUE(diff >= delay);
        EXPECT_TRUE(diff < (delay + (granularity * 2)));
    }

    w->dead = true;

    delete s;
    return 0;
}

TEST(Event, Wait)
{
    mock_setup(false);

    EventQueue eq(0);

    const int num = 5;
    int err;

    pthread_t threads[num];
    Waiter w[num];
    int t1[] = { 1000, 1000, 1000, -1 };

    for (int i = 0; i < num; i++)
    {
        w[i].eq = & eq;
        w[i].id = i;
        w[i].deltas = t1;
        w[i].count = 0;
        w[i].dead = false;

        err = pthread_create(& threads[i], 0, wait_fn, & w[i]);
        ASSERT(err == 0);
    }

    while (true)
    {
        bool dead = true;
        for (int i = 0; i < num; i++)
        {
            if (!w[i].dead)
            {
                dead = false;
            }
        }
        if (dead)
        {
            break;
        }

        d_timer_t diff = eq.check();
        IGNORE(diff);

        panglos::timer_t now = panglos::timer_now();
        mock_timer_set(now + granularity);
        usleep(1000);
    }

    // wait for all the threads to terminate
    for (int i = 0; i < num; i++)
    {
        err = pthread_join(threads[i], 0);
        ASSERT(err == 0);
    }

    mock_teardown();
}

//  FIN
