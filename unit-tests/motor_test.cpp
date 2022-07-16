
#include <stdlib.h>

#include <gtest/gtest.h>

#include <panglos/debug.h>
#include <panglos/thread.h>
#include <panglos/drivers/motor.h>

#include "mock.h"

using namespace panglos;

    /*
     *
     */

static void seek_test(Stepper *stepper, int seek)
{
    stepper->seek(seek);
    const int start = stepper->position();
    int step = 1;
    if (seek < start)
    {
        step = -1;
    }

    for (int i = start; i != seek; i += step)
    {
        EXPECT_EQ(stepper->position(), i);
        EXPECT_EQ(stepper->ready(), false);
        // step to the next position
        stepper->poll();
    }

    // should now be in position
    EXPECT_EQ(stepper->position(), seek);

    // However, because of the accel/decel, overshoot etc.
    // it may not have come to rest yet.
    while (!stepper->ready())
    {
        stepper->poll();
    }
    EXPECT_EQ(stepper->ready(), true);
}

    /*
     *
     */

static void move(Stepper *stepper, int expect, bool *zero=0)
{
    if (zero)
    {
        *zero = false;
    }

    while (true)
    {
        const int pos = stepper->position();
        if (zero && (pos == 0))
        {
            // passed through zero
            *zero = true;
        }
        if (pos == expect)
            break;
        stepper->poll();
    }
}

static void move_to(Stepper *stepper, int t)
{
    const int steps = stepper->get_steps();
    int expect = t;

    // clip to valid range
    if (t < 0)
        expect = 0;
    if (t >= steps)
        expect = steps-1;

    stepper->seek(t);
    move(stepper, expect);
}

    /*
     *
     */

TEST(Motor, Seek)
{
    mock_setup(true);

    int cycle = 5000;
    MockPin a(1), b(2), c(3), d(4);
    MotorIo_4 motor(& a, & b, & c, & d);
    Stepper stepper(cycle, & motor);

    // seek test
    EXPECT_EQ(stepper.position(), 0);
    seek_test(& stepper, 100);
    EXPECT_EQ(stepper.get_target(), 100);
    seek_test(& stepper, 0);
    seek_test(& stepper, 4999);
    seek_test(& stepper, 50);
    seek_test(& stepper, 1050);
    EXPECT_EQ(stepper.get_target(), 1050);

    // zero test
    stepper.zero();
    EXPECT_EQ(stepper.position(), 0);
    seek_test(& stepper, 100);

    // seek should clip at limits (<0)
    seek_test(& stepper, 100);
    stepper.seek(-1);
    while (!stepper.ready())
    {
        stepper.poll();
    }
    EXPECT_EQ(stepper.position(), 0);

    // seek should clip at limits (>= cycle)
    stepper.seek(cycle + 100);
    while (!stepper.ready())
    {
        stepper.poll();
    }
    EXPECT_EQ(stepper.position(), cycle - 1);

    // seek should clip at limits (>= cycle)
    stepper.seek(cycle);
    while (!stepper.ready())
    {
        printf("%d\n", stepper.position());
        stepper.poll();
    }
    EXPECT_EQ(stepper.position(), cycle - 1);

    mock_teardown();
}

    /*
     *
     */

TEST(Motor, Nowhere)
{
    mock_setup(true);

    int cycle = 5000;
    MockPin a(1), b(2), c(3), d(4);
    MotorIo_4 motor(& a, & b, & c, & d);
    Stepper stepper(cycle, & motor);

    // seek test
    EXPECT_EQ(stepper.position(), 0);

    // check it goes nowhere
    stepper.seek(0);
    stepper.poll();
    EXPECT_EQ(stepper.position(), 0);
    EXPECT_TRUE(stepper.ready());

    move_to(& stepper, 100);
    EXPECT_EQ(stepper.position(), 100);

    // should already be there
    stepper.seek(100);
    stepper.poll();
    EXPECT_EQ(stepper.position(), 100);
    EXPECT_TRUE(stepper.ready());

    mock_teardown();
}

TEST(Motor, RotateNowhere)
{
    mock_setup(true);

    int cycle = 5000;
    MockPin a(1), b(2), c(3), d(4);
    MotorIo_4 motor(& a, & b, & c, & d);
    Stepper stepper(cycle, & motor);

    // seek test
    EXPECT_EQ(stepper.position(), 0);

    // check it goes nowhere
    stepper.rotate(0);
    stepper.poll();
    EXPECT_EQ(stepper.position(), 0);
    EXPECT_TRUE(stepper.ready());

    move_to(& stepper, 100);
    EXPECT_EQ(stepper.position(), 100);

    // should already be there
    stepper.rotate(100);
    stepper.poll();
    EXPECT_EQ(stepper.position(), 100);
    EXPECT_TRUE(stepper.ready());

    mock_teardown();
}

    /*
     *  Check the state of the pins
     *
     *  They should follow a strict pattern
     *
     *  1000
     *  1001
     *  0011
     *  0010
     *  0110
     *  0100
     *  1100
     *
     *  repeated
     */

TEST(Motor, IO)
{
    mock_setup(true);

    int cycle = 5000;
    MockPin a(1), b(2), c(3), d(4);
    MotorIo_4 motor(& a, & b, & c, & d);
    Stepper stepper(cycle, & motor);

    int pins[4] = { 0, 0, 0, 0 };

    EXPECT_EQ(stepper.position(), 0);

    // starts with begining pin hi
    pins[0] = 1;
    EXPECT_TRUE(pins_match(4, 1, pins));
 
    // one pin changes state on each step
    seek_test(& stepper, 1);
    pins[3] = 1;
    EXPECT_TRUE(pins_match(4, 1, pins));

    seek_test(& stepper, 2);
    pins[0] = 0;
    EXPECT_TRUE(pins_match(4, 1, pins));

    seek_test(& stepper, 3);
    pins[2] = 1;
    EXPECT_TRUE(pins_match(4, 1, pins));

    seek_test(& stepper, 4);
    pins[3] = 0;
    EXPECT_TRUE(pins_match(4, 1, pins));

    seek_test(& stepper, 5);
    pins[1] = 1;
    EXPECT_TRUE(pins_match(4, 1, pins));

    seek_test(& stepper, 6);
    pins[2] = 0;
    EXPECT_TRUE(pins_match(4, 1, pins));

    seek_test(& stepper, 7);
    pins[0] = 1;
    EXPECT_TRUE(pins_match(4, 1, pins));

    // back to the start of the pin sequence
    seek_test(& stepper, 8);
    pins[1] = 0;
    EXPECT_TRUE(pins_match(4, 1, pins));

    mock_teardown();
}

    /*
     *
     */

TEST(Motor, Clip)
{
    mock_setup(true);
    int cycle = 5000;
    MockPin a(1), b(2), c(3), d(4);
    MotorIo_4 motor(& a, & b, & c, & d);
    Stepper stepper(cycle, & motor);

    EXPECT_EQ(stepper.position(), 0);

    // should seek to cycle-1
    move_to(& stepper, cycle + 1000);
    EXPECT_EQ(stepper.position(), cycle - 1);

    // should seek to 0
    move_to(& stepper, -1000);
    EXPECT_EQ(stepper.position(), 0);

    mock_teardown();
}

    /*
     *
     */

TEST(Motor, Rotate)
{
    mock_setup(true);
    int cycle = 5000;
    MockPin a(1), b(2), c(3), d(4);
    MotorIo_4 motor(& a, & b, & c, & d);
    Stepper stepper(cycle, & motor);

    EXPECT_EQ(stepper.position(), 0);

    stepper.seek(200);
    move(& stepper, 200);
    EXPECT_EQ(stepper.position(), 200);

    // make it go backwards, through 0
    stepper.rotate(-100);
    stepper.poll();
    EXPECT_EQ(stepper.position(), 199);
    stepper.poll();
    EXPECT_EQ(stepper.position(), 198);
    // ...

    // move to 0, on its way to the target
    move(& stepper, 0);
    EXPECT_FALSE(stepper.ready());

    // Next poll should move to cycle-1
    stepper.poll();
    EXPECT_EQ(stepper.position(), cycle-1);
    EXPECT_FALSE(stepper.ready());

    move(& stepper, cycle - 100);
    // should now have stopped moving
    EXPECT_TRUE(stepper.ready());

    mock_teardown();
}

    /*
     *
     */

static int mod(Stepper *stepper, int t)
{
    const int cycle = stepper->get_steps();

    while (t < 0)
    {
        t += cycle;
    }
    while (t >= cycle)
    {
        t -= cycle;
    }
    return t;
}

static void _quadrant(Stepper *stepper, int start, int to, bool forwards, bool crosses_zero)
{
    start = stepper->clip(start);
    to = mod(stepper, to);

    // reset to start
    stepper->seek(start);
    move(stepper, start);
    EXPECT_EQ(stepper->position(), start);

    stepper->rotate(to);

    // check the initial movement
    if (forwards)
    {
        stepper->poll();
        EXPECT_EQ(stepper->position(), mod(stepper, start+1));
        stepper->poll();
        EXPECT_EQ(stepper->position(), mod(stepper, start+2));
        // ...
    }
    else
    {
        stepper->poll();
        EXPECT_EQ(stepper->position(), mod(stepper, start-1));
        stepper->poll();
        EXPECT_EQ(stepper->position(), mod(stepper, start-2));
        // ...
    }

    bool zero;
    move(stepper, to, & zero);
    EXPECT_TRUE(stepper->ready());
    EXPECT_EQ(zero, crosses_zero);    
}

static void quadrant(Stepper *stepper, int start, bool q1, bool q2, bool q3)
{
    _quadrant(stepper, start, start + 179, true, q1);
    _quadrant(stepper, start, start + 180, false, q2);
    _quadrant(stepper, start, start + 181, false, q2);
    _quadrant(stepper, start, start + 270, false, q3);
}

TEST(Motor, RotateQuadrants)
{
    mock_setup(true);
    int cycle = 360;
    MockPin a(1), b(2), c(3), d(4);
    MotorIo_4 motor(& a, & b, & c, & d);
    Stepper stepper(cycle, & motor);

    EXPECT_EQ(stepper.position(), 0);

    quadrant(& stepper, 45, false, true, true);
    quadrant(& stepper, 45+90, false, true, false);
    quadrant(& stepper, 45+180, true, false, false);
    quadrant(& stepper, 45+270, true, false, false);

    mock_teardown();
}

TEST(Motor, RotateZero)
{
    mock_setup(true);
    int cycle = 360;
    MockPin a(1), b(2), c(3), d(4);
    MotorIo_4 motor(& a, & b, & c, & d);
    Stepper stepper(cycle, & motor);

    EXPECT_EQ(stepper.position(), 0);

    move_to(& stepper, 100);
    EXPECT_EQ(stepper.position(), 100);

    // check that ready() false through the seek() position
    stepper.rotate(0);
    stepper.seek(99);
    stepper.poll();
    EXPECT_EQ(stepper.position(), 99);
    EXPECT_FALSE(stepper.ready());

    while (stepper.position() != 0)
    {
        stepper.poll();
    }

    mock_teardown();
}

TEST(Motor, SetZero)
{
    mock_setup(true);
    int cycle = 360;
    MockPin a(1), b(2), c(3), d(4);
    MotorIo_4 motor(& a, & b, & c, & d);
    Stepper stepper(cycle, & motor);

    EXPECT_EQ(stepper.position(), 0);

    stepper.zero(100);
    EXPECT_EQ(stepper.position(), 100);
    EXPECT_TRUE(stepper.ready());

    stepper.zero();
    EXPECT_EQ(stepper.position(), 0);
    EXPECT_TRUE(stepper.ready());

    mock_teardown();
}

    /*
     *
     */

TEST(Motor, Power)
{
    mock_setup(true);
    int cycle = 360;
    MockPin a(1), b(2), c(3), d(4);
    MockPin *pp[] = { & a, & b, & c, & d };
    MotorIo_4 motor(& a, & b, & c, & d);
    Stepper stepper(cycle, & motor);

    stepper.zero(100);
    EXPECT_EQ(stepper.position(), 100);
    EXPECT_TRUE(stepper.ready());

    int pins[4] = { 0, 0, 0, 0 };

    for (int i = 0; i < 4; i++)
    {
        pins[i] = pp[i]->get();
    }

    stepper.power(false);

    int zero[4] = { 0, 0, 0, 0 };

    // all pins low
    EXPECT_TRUE(pins_match(4, 1, zero));

    stepper.power(true);
    // pins restored
    EXPECT_TRUE(pins_match(4, 1, pins));

    mock_teardown();
}

    /*
     *
     */

TEST(Motor, AccelDecel)
{
    Accelerator accel(100, 0, 5);

    int v;

    EXPECT_TRUE(accel.stopped());
    EXPECT_TRUE(accel.forward());

    // check accel forwards
    v = accel.go(true);
    EXPECT_EQ(v, 0);
    v = accel.go(true);
    EXPECT_EQ(v, 20);
    v = accel.go(true);
    EXPECT_EQ(v, 40);
    v = accel.go(true);
    EXPECT_EQ(v, 60);
    v = accel.go(true);
    EXPECT_EQ(v, 80);
    v = accel.go(true);
    EXPECT_EQ(v, 100);
    EXPECT_FALSE(accel.stopped());
    EXPECT_TRUE(accel.forward());

    // should stay at max
    v = accel.go(true);
    EXPECT_EQ(v, 100);
    EXPECT_FALSE(accel.stopped());
    EXPECT_TRUE(accel.forward());

    // decelerate
    accel.decelerate();
    EXPECT_FALSE(accel.stopped());
    EXPECT_TRUE(accel.forward());
 
    v = accel.go(true);
    EXPECT_EQ(v, 80);
    v = accel.go(true);
    EXPECT_EQ(v, 60);
    v = accel.go(true);
    EXPECT_EQ(v, 40);
    v = accel.go(true);
    EXPECT_EQ(v, 20);
    // should be stopped now
    EXPECT_TRUE(accel.stopped());
    EXPECT_TRUE(accel.forward());

    // start it going again. should start with min_p
    v = accel.go(true);
    EXPECT_EQ(v, 0);
    EXPECT_TRUE(accel.forward());

    v = accel.go(true);
    EXPECT_EQ(v, 20);
    v = accel.go(true);
    EXPECT_EQ(v, 40);
    v = accel.go(true);
    EXPECT_EQ(v, 60);
    EXPECT_TRUE(accel.forward());

    // now drive it the opposite direction
    EXPECT_TRUE(accel.forward());
    v = accel.go(false);
    EXPECT_EQ(v, 40);
    // still moving forward
    EXPECT_TRUE(accel.forward());
    v = accel.go(false);
    EXPECT_EQ(v, 20);
    EXPECT_TRUE(accel.forward());
    v = accel.go(false);
    EXPECT_EQ(v, 0);
    EXPECT_TRUE(accel.stopped());
    EXPECT_FALSE(accel.forward());

    // we've come to a stop. Should continue to accelerate in reverse direction
    v = accel.go(false);
    EXPECT_EQ(v, 0);
    v = accel.go(false);
    EXPECT_EQ(v, 20);
    v = accel.go(false);
    EXPECT_EQ(v, 40);
    v = accel.go(false);
    EXPECT_EQ(v, 60);
    v = accel.go(false);
    EXPECT_EQ(v, 80);
    v = accel.go(false);
    EXPECT_EQ(v, 100);

    v = accel.go(false);
    EXPECT_EQ(v, 100);
    v = accel.go(false);
    EXPECT_EQ(v, 100);

    // go forward again
    // decelerate ...
    EXPECT_FALSE(accel.forward());
    v = accel.go(true);
    EXPECT_EQ(v, 80);
    EXPECT_FALSE(accel.forward());
    v = accel.go(true);
    EXPECT_EQ(v, 60);
    v = accel.go(true);
    EXPECT_EQ(v, 40);
    v = accel.go(true);
    EXPECT_EQ(v, 20);
    // stop ..
    v = accel.go(true);
    EXPECT_EQ(v, 0);
    EXPECT_TRUE(accel.forward());
    v = accel.go(true);
    EXPECT_EQ(v, 0);
    v = accel.go(true);
    EXPECT_EQ(v, 20);
    v = accel.go(true);
    EXPECT_EQ(v, 40);
    v = accel.go(true);
    EXPECT_EQ(v, 60);
    v = accel.go(true);
    EXPECT_EQ(v, 80);
    v = accel.go(true);
    EXPECT_EQ(v, 100);
    v = accel.go(true);
    EXPECT_EQ(v, 100);
    // up to max speed

    // decelerate until we've stopped
    while (!accel.stopped())
    {
        v = accel.go(false);
    }
    EXPECT_EQ(v, 0);
    EXPECT_FALSE(accel.forward());

    // go forward from STOP
    v = accel.go(true);
    EXPECT_EQ(v, 0);
    EXPECT_TRUE(accel.forward());
    v = accel.go(true);
    EXPECT_EQ(v, 20);
    v = accel.go(true);
    EXPECT_EQ(v, 40);
}

TEST(Motor, Near)
{
    mock_setup(true);
    int cycle = 360;
    MockPin a(1), b(2), c(3), d(4);
    MotorIo_4 motor(& a, & b, & c, & d);
    Stepper stepper(cycle, & motor, 1000, 10000, 20);

    Accelerator *acc = stepper.accelerator;

    EXPECT_EQ(0, stepper.position());
    EXPECT_EQ(Accelerator::STOP, acc->get_state());
    stepper.seek(9);
    stepper.poll();
    EXPECT_EQ(1, stepper.position());
    EXPECT_EQ(Accelerator::ACCEL, acc->get_state());
    stepper.poll();
    EXPECT_EQ(2, stepper.position());
    EXPECT_EQ(Accelerator::ACCEL, acc->get_state());
    stepper.poll();
    EXPECT_EQ(3, stepper.position());
    EXPECT_EQ(Accelerator::ACCEL, acc->get_state());
    stepper.poll();
    EXPECT_EQ(4, stepper.position());
    EXPECT_EQ(Accelerator::ACCEL, acc->get_state());
    stepper.poll();
    EXPECT_EQ(5, stepper.position());
    EXPECT_EQ(Accelerator::ACCEL, acc->get_state());

    // should start decelerating
    stepper.poll();
    EXPECT_EQ(6, stepper.position());
    EXPECT_EQ(Accelerator::DECEL, acc->get_state());
    stepper.poll();
    EXPECT_EQ(7, stepper.position());
    EXPECT_EQ(Accelerator::DECEL, acc->get_state());
    stepper.poll();
    EXPECT_EQ(8, stepper.position());
    EXPECT_EQ(Accelerator::DECEL, acc->get_state());
    stepper.poll();
    EXPECT_EQ(9, stepper.position());
    EXPECT_EQ(Accelerator::STOP, acc->get_state());

    mock_teardown();
}

TEST(Motor, Overshoot)
{
    mock_setup(true);
    int cycle = 4096;
    MockPin a(1), b(2), c(3), d(4);
    MotorIo_4 motor(& a, & b, & c, & d);
    Stepper stepper(cycle, & motor, 1000, 10000, 20);

    stepper.seek(2000);
    while (stepper.position() < 1000)
    {
        //PO_DEBUG("p=%d", stepper.position());
        stepper.poll();
    }

    EXPECT_EQ(1000, stepper.position());

    // change that target to a few counts away ..
    stepper.seek(1005);

    int max_p = 0;
    while (!stepper.ready())
    {
        const int p = stepper.position();
        if (p > max_p)
        {
            max_p = p;
        }
        //PO_DEBUG("p=%d", p);
        stepper.poll();
    }
    EXPECT_EQ(1005, stepper.position());
    // check it overshot
    EXPECT_TRUE(max_p > 1010);
 
    stepper.seek(3000);
    while (stepper.position() < 2000)
    {
        //PO_DEBUG("p=%d", stepper.position());
        stepper.poll();
    }

    EXPECT_EQ(2000, stepper.position());

    // change target to a few counts back ..
    stepper.seek(1995);

    max_p = 0;
    while (!stepper.ready())
    {
        const int p = stepper.position();
        if (p > max_p)
        {
            max_p = p;
        }
        //PO_DEBUG("p=%d", p);
        stepper.poll();
    }
    EXPECT_EQ(1995, stepper.position());
    // check it overshot
    EXPECT_TRUE(max_p > 2010);

    mock_teardown();
}

#if 1

    /*
     *
     */

#include <panglos/select.h>
#include <panglos/event.h>
#include <panglos/timer.h>

    /*
     *
     */

class Waiter
{
    Waiter *next;

public:
    Event event;

    Waiter(Semaphore *s)
    :   next(0),
        event(s, 0)
    {
    }

    virtual ~Waiter() {}

    static Waiter ** next_fn(Waiter *w) { return & w->next; }

    virtual int visit(Semaphore *s) = 0;
};

    /*
     *
     */

class Scheduler 
{
    bool dead;
    Semaphore *sem_kill;

    Mutex *mutex;
    typedef List<Waiter*> Waiters;

    Waiters waiters;
    Select *select;

    EventQueue *eq;

public:
    Scheduler(EventQueue *q, int size)
    :   dead(false), 
        sem_kill(0),
        mutex(0),
        waiters(Waiter::next_fn),
        select(0),
        eq(q)
    {
        mutex = Mutex::create();
        select = new Select(size+1);
        sem_kill = Semaphore::create();
        select->add(sem_kill);
    }

    ~Scheduler()
    {
        delete mutex;
        select->remove(sem_kill);
        delete sem_kill;
        delete select;
    }

    void add(Waiter *w)
    {
        waiters.push(w, mutex);
    }

    void add(Semaphore *s)
    {
        select->add(s);
    }

    void remove(Waiter *w)
    {
        waiters.remove(w, mutex);
        // remove any pending event in the event queue
        eq->remove(& w->event);
    }

    void remove(Semaphore *s)
    {
        select->remove(s);
    }

    static int visit(Waiter *w, void *arg)
    {
        ASSERT(w);
        ASSERT(arg);
        Semaphore *s = (Semaphore *) arg;
        return w->visit(s);
    }

    void run()
    {
        while (!dead)
        {
            Semaphore *s = select->wait();
            if ((s == 0) || (s == sem_kill))
            {
                continue;
            }

            // check all the waiters for this semaphore
            waiters.visit(visit, s, mutex);
        }

        // remove all the waiters
        while (true)
        {
            Waiter *w = waiters.pop(mutex);
            if (!w)
            {
                break;
            }
            remove(w);
        }
    }

    void kill()
    {
        dead = true;
        sem_kill->post();
    }

    void schedule(Waiter *w, panglos::timer_t abs_time)
    {
        // remove any pending event
        eq->remove(& w->event);

        // schedule the event
        w->event.time = abs_time;
        eq->add(& w->event);
    }
};

    /*
     *
     */

class MotorThread : public Waiter
{
    Scheduler *scheduler;
    panglos::timer_t next_time;
    panglos::timer_t period;

public:

    MotorThread(Scheduler *sch, Semaphore *s, panglos::timer_t _period)
    :   Waiter(s),
        scheduler(sch),
        next_time(timer_now()),
        period(_period)
    {
        ASSERT(scheduler);
        ASSERT(s);
        scheduler->add(this);
        scheduler->add(s);

        scheduler->schedule(this, next_time);
    }

    ~MotorThread()
    {
        scheduler->remove(this);
        scheduler->remove(event.semaphore);
    }

    void execute()
    {
        PO_DEBUG("%d", timer_now());
        next_time += period;
        scheduler->schedule(this, next_time);
    }

    virtual int visit(Semaphore *s)
    {
        if (s == event.semaphore)
        {
            execute();
            return 1;
        }

        // not us
        return 0;
    }
};

    /*
     *
     */

static void run_sched(void *arg)
{
    ASSERT(arg);
    Scheduler *sched = (Scheduler*) arg;
    sched->run();
}

TEST(Schedule, Test)
{
    mock_setup(true);
    Scheduler scheduler(& event_queue, 10);

    Thread *thread = Thread::create("xx");
    thread->start(run_sched, & scheduler);

    Semaphore *s = Semaphore::create();
    MotorThread *m = new MotorThread(& scheduler, s, 2000000);

    sleep(1);
    scheduler.kill();

    thread->join();
    delete thread;

    delete m;
    delete s;

    mock_teardown();
}

#endif

//  FIN
