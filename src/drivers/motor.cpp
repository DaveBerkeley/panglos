
#include <math.h>
#include <stdlib.h>

#include "panglos/debug.h"

#include "panglos/drivers/gpio.h"
#include "panglos/drivers/motor.h"

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

void Accelerator::show_state(const char *t)
{
#if 0
    static const Code lut[] = { 
        { STOP,  "STOP ", },
        { ACCEL, "ACCEL", },
        { DECEL, "DECEL", },
        { FULL,  "FULL ", },
        { 0, 0 },
    };
    const char *dir = dirn_forward ? ">>" : "<<";
    if (state == STOP)
    {
        dir = "--";
    }

    PO_REPORT("%s state=%s %s idx=%d", t, err_lookup(lut, state), dir, idx);
#else
    IGNORE(t);
#endif
}

void Accelerator::set_state(State s)
{
    state = s;
    show_state("set");
}

Accelerator::Accelerator(int _max_p, int _min_p, int _steps)
:   size(_steps),
    idx(0),
    state(STOP),
    dirn_forward(true),
    table(0),
    max_p(_max_p),
    min_p(_min_p)
{
    set_state(STOP);

    table = new int[size];

    for (int i = 0; i < size; i++)
    {
        const int range = max_p - min_p;
        const float div = (float) range / float(size);
        const int v = min_p + i * (int) div;
        table[i] = v;
        ASSERT_ERROR(v >= 0, "i=%d v=%d", i, v);
    }
}

Accelerator::~Accelerator()
{
    delete[] table;
}

void Accelerator::decelerate()
{
    if (state != STOP)
    {
        set_state(DECEL);
    }
}

int Accelerator::go(int up)
{
    show_state(up ? ">> " : "<< ");

    // if we are stopped, the current direction does not matter
    if (state == STOP)
    {
        // accelerate in the direction requested
        dirn_forward = up;
        set_state(ACCEL);
        return min_p;
    }
 
    if (dirn_forward == up)
    {
        // already going in this direction
        if (state == FULL)
        {
            // already at full speed
            return max_p;
        }
        if (state == ACCEL)
        {
            if ((idx+1) == size)
            {
                // reached full speed
                set_state(FULL);
                return max_p;
            }
            // continue acceleration
            idx += 1;
            return table[idx];
        }
        if (state == DECEL)
        {
            // continue decelerating
            int v = table[idx];
            if (idx > 0)
            {
                idx -= 1;
            }
            if (idx == 0)
            {
                // come to a stop
                set_state(STOP);
            }
            return v;
        }
        ASSERT(0);
    }

    // changing direction
    ASSERT(dirn_forward != up);

    if (state == FULL)
    {
        // need to decel and come to a stop
        ASSERT(idx == (size-1));
        set_state(DECEL);
        return table[idx];
    }
    if (state == ACCEL)
    {
        // we need to reverse direction
        set_state(DECEL);
        if (idx > 0)
        {
            idx -= 1;
        }
        if (idx == 0)
        {
            set_state(STOP);
            dirn_forward = up;
        }
        return table[idx];
    }
    if (state == DECEL)
    {
        if (idx > 0)
        {
            idx -= 1;
        }
        if (idx == 0)
        {
            set_state(STOP);
            dirn_forward = up;
        }
        return table[idx];
    }
    ASSERT(0);
    return 0;
}

  /*
  *
  */

Stepper::Stepper(int cycle, MotorIo *io, uint32_t time, int32_t slow, int _steps)
:   io(io),
    steps(cycle), 
    count(0), 
    target(0), 
    rotate_to(-1), 
    period(time), 
    accelerator(0)
{
    ASSERT(io);
    accelerator = new Accelerator(time, (slow == -1) ? time : slow, _steps);
}

Stepper::~Stepper()
{
    delete accelerator;
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

void Stepper::seek(int t)
{
    target = clip(t);
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
    }
}

int Stepper::get_target()
{
    return target;
}

bool Stepper::ready()
{
    return (get_delta() == 0) && (accelerator->get_idx() == 0);
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

int Stepper::poll()
{
    const int delta = get_delta();

    if ((delta == 0) && (accelerator->stopped()))
    {
        return 0;
    }

    Accelerator::State state = accelerator->get_state();

    if ((state == Accelerator::FULL) || (state == Accelerator::ACCEL))
    {
        if (abs(delta) <= (accelerator->get_idx()))
        {
            accelerator->decelerate();
        }
    }

    const bool dirn = delta > 0;
    const int period = accelerator->go(dirn);
    // we may still be travelling in the opposite direction ..
    const bool travel = accelerator->forward();

    step(travel);

    if ((rotate_to != -1) && (rotate_to == count) && accelerator->stopped())
    {
        //  we've arrived
        rotate_to = -1;
        target = count;
    }

    return period;
}

}   //  namespace panglos

// FIN
