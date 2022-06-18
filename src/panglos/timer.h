
#if !defined(__PANGLOS_TIMER__)
#define __PANGLOS_TIMER__

#include <stdint.h>

namespace panglos {

class Timer
{
public:
    enum ID {
        TIMER_1 = 0,
        TIMER_2,
        TIMER_3,
        TIMER_4,
        TIMER_5,
        TIMER_6,
        TIMER_7,
        TIMER_8,
        TIMER_9,
        TIMER_10,
        TIMER_11,
        TIMER_12,
        TIMER_13,
        TIMER_14,
        TIMER_15,
    };

    enum Chan {
        CHAN_NONE,
        CHAN_1,
        CHAN_2,
        CHAN_3,
        CHAN_4,
    };

    enum Trigger {
        TRIG_NONE,
        TRIG_UPDATE,
        TRIG_1,
        TRIG_2,
        TRIG_3,
        TRIG_4,
    };

    virtual void init(uint32_t prescaler, uint32_t period) = 0;
    virtual void set_ms_mode(bool on=false, Trigger out_trig=TRIG_NONE) = 0;
    virtual void start() = 0;

    virtual void enable_dma() = 0;
    virtual void enable_dma(Chan chan) = 0;
    virtual void start_pwm(Chan chan, uint32_t value) = 0;
    virtual void start_oc(Chan chan) = 0;
    virtual void event() = 0;

    virtual ID get_id() = 0;

    virtual uint32_t get_counter() = 0;
    
    static Timer *create(ID id);
    static void alloc(ID id);
};

    /*
     *
     */

typedef uint32_t timer_t;
typedef int32_t d_timer_t;

timer_t timer_now();
void timer_set(d_timer_t dt);
void timer_wait(d_timer_t dt);
void timer_init();

#define TIMER_MS    (10)
#define TIMER_S     (1000 * TIMER_MS)

}   //  namespace panglos

#endif // __PANGLOS_TIMER__

//  FIN
