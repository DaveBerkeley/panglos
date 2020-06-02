
#if !defined(__MOTOR_H__)
#define __MOTOR_H__

    /*
     *
     */

#include <stdint.h>

#include "gpio.h"
#include "mutex.h"

namespace panglos {

    /*
     *  Abstract Stepper Interface
     */

class MotorIo
{
public:
    virtual void step(bool up) = 0;
    virtual void power(bool on) = 0;
};

    /*
     *  Four wire uni-polar stepper
     */

class MotorIo_4 : public MotorIo
{
    const static int PINS = 4;
    GPIO *pins[PINS];

    const static int STATES = 8;
    const static int cycle[STATES][PINS];

    int state;

    bool needs_flush;

    void set_state(int s);
    void flush();

public:
    MotorIo_4(GPIO * p1, GPIO * p2, GPIO * p3, GPIO * p4, bool flush=false);

    virtual void step(bool up);
    virtual void power(bool on);
};

    /*
     *
     */

class Accelerator
{
    int size;
    int idx;
public:
    enum State { STOP=0, ACCEL=1, DECEL=2, FULL=3 };
private:
    enum State state;
    bool dirn_forward;
    int *table;

    int max_p;
    int min_p;

    void show_state(const char *t);
    void set_state(State s);

public:
    Accelerator(int _max_p, int _min_p, int _steps);
    ~Accelerator();

    void decelerate();
    int go(int up);

    int get_size() { return size; }
    int get_idx() { return idx; }
    State get_state() { return state; }
    bool stopped() { return state == STOP; }
    bool forward() { return dirn_forward; }
};

   /*
    *
    */

class Stepper
{
    MotorIo *io;
    int steps;
    int count;
    int target;
    int rotate_to;
    uint32_t period;
    Semaphore *semaphore;

public:
    Accelerator *accelerator;
private:

    void set_state(int s);
    void step(bool up);
    int get_delta();

    virtual void pause(uint32_t us);

public:
    Stepper(int cycle, MotorIo* io, uint32_t time=1000, int32_t slow=-1, int steps=-1);
    virtual ~Stepper();

    virtual int position();
    virtual void seek(int t);
    virtual void rotate(int t);
    virtual int get_target();
    virtual int get_steps();
    virtual int clip(int t);
    virtual bool ready();
    virtual void zero(int t=0);
    virtual void set_steps(int s);
    virtual bool poll();
    virtual void power(bool on);
};

}   //  namespace panglos

#endif // __MOTOR_H__

// FIN
