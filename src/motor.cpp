
#include <math.h>

#include "panglos/debug.h"

#include "panglos/gpio.h"
#include "panglos/event.h"
#include "panglos/motor.h"

namespace panglos {

    /*
     *
     */

const int MotorIo_4::cycle[STATES][PINS] = {
    { 1, 0, 0, 0 },
    { 1, 0, 0, 1 },
    { 0, 0, 0, 1 },
    { 0, 0, 1, 1 },
    { 0, 0, 1, 0 },
    { 0, 1, 1, 0 },
    { 0, 1, 0, 0 },
    { 1, 1, 0, 0 },
};

MotorIo_4::MotorIo_4(GPIO *p1, GPIO *p2, GPIO *p3, GPIO *p4, bool flush)
: needs_flush(flush)
{
    pins[0] = p1;
    pins[1] = p2;
    pins[2] = p3;
    pins[3] = p4;

    set_state(0);
}

void MotorIo_4::flush()
{
    if (needs_flush)
    {
        for (int i = 0; i < PINS; i++)
        {
            pins[i]->flush();
        }
    }
}

void MotorIo_4::set_state(int s)
{
    const int* states = cycle[s];
    for (int i = 0; i < PINS; i++)
    {
        pins[i]->set(states[i]);
    }
    state = s;
    flush();
}

void MotorIo_4::step(bool up)
{
    // calculate next state
    int delta = up ? 1 : -1;
    int s = state + delta;
    if (s < 0)
        s += STATES;
    if (s >= STATES)
        s-= STATES;
    set_state(s);
}

void MotorIo_4::power(bool on)
{
    if (on)
    {
        set_state(state);
    }
    else
    {
        for (int i = 0; i < PINS; i++)
        {
            pins[i]->set(false);
        }
        flush();
    }
}

  /*
  *
  */

Stepper::Stepper(int cycle, MotorIo *io, uint32_t time)
:   io(io),
    steps(cycle), 
    count(0), 
    target(0), 
    rotate_to(-1), 
    period(time), 
    semaphore(0),
    accel(NONE), 
    reference(0)
{
    ASSERT(io);
    semaphore = Semaphore::create();
}

Stepper::~Stepper()
{
    delete semaphore;
}

void Stepper::step(bool up)
{
    // calculate next state
    io->step(up);

    int delta = up ? 1 : -1;
    int c = count + delta;
    if (c < 0)
        c += steps;
    if (c >= steps)
        c -= steps;
    count = c;
}

void Stepper::power(bool on)
{
    io->power(on);
}

int Stepper::position()
{
    return count;
}

int Stepper::clip(int t)
{
    // clip to valid limits
    if (t < 0)
    {
        return 0;
    }

    if (t >= steps)
    {
        return steps - 1;
    }

    return t;
}

void Stepper::set_accel()
{

    int delta = get_delta();
    if (!delta)
    {
        return;
    }

    reference = count;
    accel = ACCEL;
}

void Stepper::seek(int t)
{
    target = clip(t);
    set_accel();
}

void Stepper::rotate(int t)
{
    while (t < 0)
    {
        t += steps;
    }

    t %= steps;

    // only set if we're aren't there already
    if (t != count)
    {
        rotate_to = t;
        set_accel();
    }
}

int Stepper::get_target()
{
    return target;
}

bool Stepper::ready()
{
    return get_delta() == 0;
}

void Stepper::zero(int t)
{
    target = t;
    count = t;
}

void Stepper::set_steps(int s)
{
    steps = s;
}

int Stepper::get_steps()
{
    return steps;
}

int Stepper::get_delta()
{
    if (rotate_to == -1)
    {
        return target - count;
    }

    // which direction to move?
    int d = rotate_to - count;
    if (d == 0)
    {
        return 0;
    }

    const int half = steps / 2;

    if (d > 0)
    {
        // d is +ve
        if (d < half)
        {
            // move forward
            return d;
        }
        // move backwards
        return -d;
    }

    // d is -ve
    if (d >= -half)
    {
        // move backwards
        return d;
    }

    // move forward
    return -d;
}

void Stepper::pause(uint32_t us)
{
    panglos::event_queue.wait(semaphore, us);
}

bool Stepper::poll()
{
    int delta = get_delta();

    if (delta == 0)
    {
        return false;
    }

    step(delta > 0);

    if ((rotate_to != -1) && (rotate_to == count))
    {
        //  we've arrived
        rotate_to = -1;
        target = count;
    }

    static const int num = 20;
    static uint32_t speed[num];
    static bool init = false;

    if (!init)
    {
        // initialise the accel / decel profile.
        init = true;
        for (int i = 0; i < num; i++)
        {
            const uint32_t fast = period;
            const uint32_t slow = period * 10;
            uint32_t s = ((i * fast) + ((num - i) * slow)) / num;
            speed[i] = s;
        }
    }

    int move = abs(delta);

    if (move < num)
    {
        // looks like decel
        accel = DECEL;
        pause(speed[move]);
        return true;
    }

    if (accel == ACCEL)
    {
        int move = abs(reference - count);
        if (move < num)
        {
            pause(speed[move]);
            return true;
        }
    }

    accel = NONE;
    pause(period);
    return true;
}

}   //  namespace panglos

// FIN
