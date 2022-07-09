
#include <sys/select.h>

#include <gtest/gtest.h>

#include <panglos/debug.h>
#include <panglos/event.h>
#include <panglos/timer.h>
#include <panglos/thread.h>

#include "mock.h"

#define PIN_MIN 0
#define PIN_MAX 14

using namespace panglos;

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

static Thread *thread;
static bool dead;
static bool running;

void event_task(void *arg)
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
    thread = Thread::create("event");
    thread->start(event_task, 0);
}

void mock_teardown()
{
    dead = true;

    if (running)
    {
        thread->join();
        delete thread;
        thread = 0;
    }
}

    /*
     *  GPIO
     */

MockPin::MockPin(int p, enum Irq _change)
:   pin(p),
    change(_change),
    irq_fn(0),
    irq_arg(0)
{
    ASSERT((pin >= PIN_MIN) && (pin <= PIN_MAX));
}

void MockPin::set(bool state)
{
    const bool same = pins[pin] == state;
    pins[pin] = state;

    if (!irq_fn)
        return;
    if (same)
        return;
    if (change == NONE)
        return;
    if ((change == CHANGE) 
            || ((change == SET) && state)
            || ((change == CLR) && !state))
    {
        irq_fn(irq_arg);
    }
}

bool MockPin::get()
{
    return pins[pin];
}

void MockPin::toggle()
{
    set(!get());
}

void MockPin::set_interrupt_handler(enum Interrupt irq, void (*fn)(void*), void *arg)
{
    IGNORE(irq); // TODO
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

    /*
     *
     */

MockI2C::MockI2C()
: I2C(0)
{
    memset(regs, 0, sizeof(regs));
}

bool MockI2C::probe(uint8_t addr, uint32_t timeout)
{
    IGNORE(addr);
    IGNORE(timeout);
    ASSERT(0);
    return false;
}

int MockI2C::write(uint8_t addr, const uint8_t* wr, uint32_t len)
{
    IGNORE(addr);
    ASSERT(len);
    int reg = wr[0];
    memcpy(& regs[reg], & wr[1], len-1);
    PO_DEBUG("%#x %#x %#x", addr, wr[0], wr[1]);
    return len;
}

int MockI2C::write_read(uint8_t addr, const uint8_t* wr, uint32_t len_wr, uint8_t* rd, uint32_t len_rd)
{
    IGNORE(addr);
    ASSERT(len_wr);
    int reg = wr[0];
    ASSERT(len_wr == 1);
    ASSERT(len_rd == 1);
    PO_DEBUG("%#x %#x %#x", addr, reg, regs[reg]);
    memcpy(& regs[reg], & wr[1], len_wr-1);
    memcpy(rd, & regs[reg], len_rd);
    return len_wr + len_rd;
}

int MockI2C::read(uint8_t addr, uint8_t* rd, uint32_t len)
{
    IGNORE(addr);
    IGNORE(rd);
    ASSERT(0);
    return len;
}

    /*
     *
     */

static bool time_auto = true;
static panglos::Time::tick_t time_now = 0;

void time_set(panglos::Time::tick_t t)
{
    time_now = t;
}

bool time_set_auto(bool on)
{
    const bool was = time_auto;
    time_auto = on;
    return was;
}

namespace panglos {

panglos::Time::tick_t Time::get()
{
    if (time_auto)
    {
        // TODO : return now
        ASSERT(0);
    }

    return time_now;
}

}

//  FIN
