
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

class Stepper
{
    MotorIo *io;
    int steps;
    int count;
    int target;
    int rotate_to;
    uint32_t period;
    Semaphore *semaphore;

    enum Accel { ACCEL, DECEL, NONE };
    enum Accel accel;
    int reference;

    void set_state(int s);
    void step(bool up);
    int get_delta();
    void set_accel();

    void pause(uint32_t us);

public:
    Stepper(int cycle, MotorIo* io, uint32_t time=1000);
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
