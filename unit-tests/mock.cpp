
#include <pthread.h>
#include <sys/select.h>

#include <gtest/gtest.h>

#include <panglos/debug.h>
#include <panglos/event.h>
#include <panglos/timer.h>

#include "mock.h"

#define PIN_MIN 0
#define PIN_MAX 14

#define IGNORE(x) ((x) = (x))

//using namespace panglos;

void xxx(){}

MockPin led(1);
MockPin *err_led = & led;

namespace panglos {

//Global event queue
EventQueue event_queue(0);

}   //  namespace panglos

static int pins[PIN_MAX];
static uint64_t elapsed_us;

static panglos::timer_t cycles = 0;

void mock_timer_set(panglos::timer_t t)
{
    cycles = t;
}

namespace panglos {

panglos::timer_t timer_now()
{
    return cycles;
}

void timer_wait(d_timer_t dt)
{
    IGNORE(dt);
}

void timer_init()
{
}

} // namespace panglos

static pthread_t thread;
static bool dead;
static bool running;

void *event_task(void *arg)
{
    IGNORE(arg);

    while (!dead)
    {
        panglos::d_timer_t dt = panglos::event_queue.check();
        usleep(1);
        cycles += 1000;
        //printf("%d %d\n", cycles, dt);
        IGNORE(dt);
    }
    return 0;
}

void mock_setup(bool event_thread)
{
    mock_timer_set(0);
    memset(pins, 0, sizeof(pins));
    elapsed_us = 0;

    running = event_thread;

    if (!running)
    {
        return;
    }

    dead = false;
    int err = pthread_create(& thread, 0, event_task, 0);
    EXPECT_EQ(0, err);
}

void mock_teardown()
{
    dead = true;

    if (running)
    {
        int err = pthread_join(thread, 0);
        EXPECT_EQ(0, err);
    }
}

    /*
     *  GPIO
     */

MockPin::MockPin(int p)
: pin(p)
{
    ASSERT((pin >= PIN_MIN) && (pin <= PIN_MAX));
}

void MockPin::set(bool state)
{
    pins[pin] = state;
}

bool MockPin::get()
{
    return pins[pin];
}

void MockPin::toggle()
{
    set(!get());
}

void MockPin::on_interrupt()
{
}

static void (*irq_fn)(void*);
static void *irq_arg;

void MockPin::set_interrupt_handler(void (*fn)(void*), void *arg)
{
    irq_fn = fn;
    irq_arg = arg;
}


    /*
     *  IO
     */

bool pins_match(int num, int start, const int *p)
{
    ASSERT(start >= PIN_MIN);
    ASSERT((start + num) < PIN_MAX);

    for (int i = 0; i < num; i++)
    {
        if (pins[i+start] != p[i])
        {
            printf("%d: %d %d\n", i, pins[i+start], p[i]);
            return false;
        }
    }

    return true;
}

//  FIN
