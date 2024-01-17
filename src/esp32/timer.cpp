
#if defined(ESP32)

#include <esp_timer.h>

#include "panglos/debug.h"

#include "panglos/hal.h"

#include "panglos/drivers/timer.h"
#include "panglos/esp32/timer.h"

    /*
     *
     */

#define esp_check(err) ASSERT_ERROR((err) == ESP_OK, "err=%s", lut(panglos::err_lut, (err)));

namespace panglos {

    /*
     *
     */

class ESP_Timer : public Timer
{
    esp_timer_handle_t handle;
    uint64_t period;
    void (*handler)(Timer *, void *);
    void *arg;
public:
    ESP_Timer();
    ~ESP_Timer();

    virtual void start(bool periodic) override;
    virtual void stop()  override;
    virtual void set_period(Period p) override;
    virtual void set_handler(void (*fn)(Timer *, void *), void *arg) override;
    virtual Period get() override;

    static void event_cb(void *arg);
};

    /*
     *
     */

ESP_Timer::ESP_Timer()
:   handle(0),
    period(0),
    handler(0),
    arg(0)
{
    esp_timer_create_args_t def = {
        .callback = event_cb,
        .arg = this,
        .dispatch_method = ESP_TIMER_TASK, // ISRs no longer supported!
        .name = "TODO",
        .skip_unhandled_events = false
    };
    esp_err_t err = esp_timer_create(& def, & handle);
    esp_check(err);
}

ESP_Timer::~ESP_Timer()
{
    esp_err_t err;
    stop();
    err = esp_timer_delete(handle);
    esp_check(err);
}

    /*
     *
     */

void ESP_Timer::start(bool periodic)
{
    esp_err_t err;

    ASSERT(period); // must have period set before start
   
    if (periodic)
    {
        err = esp_timer_start_periodic(handle, period);
    }
    else
    {
        err = esp_timer_start_once(handle, period);
    }
    esp_check(err);
}

void ESP_Timer::stop()
{
    esp_err_t err = esp_timer_stop(handle);
    if (err != ESP_ERR_INVALID_STATE)
    {
        esp_check(err);
    }
}

void ESP_Timer::set_period(Period p)
{
    period = p;
    // TODO : do we need to stop / start the timer?
}

    /*
     *
     */

void ESP_Timer::set_handler(void (*fn)(Timer *, void *), void *_arg)
{
    handler = fn;
    arg = _arg;
}

void ESP_Timer::event_cb(void *arg)
{
    ASSERT(arg);
    ESP_Timer *timer = (ESP_Timer*) arg;
    if (timer->handler)
    {
        timer->handler(timer, timer->arg);
    }
}

    /*
     *
     */

Timer::Period ESP_Timer::get()
{
    PO_ERROR("TODO");
    ASSERT(0);
    return 0;
}

    /*
     *
     */

Timer *Timer::create()
{
    return new ESP_Timer;
}

    /*
     *  High Res Timer using hardware, not the esp-idf thread based timer
     */

#include "driver/timer.h"

#define CHECK(err) esp_check(err);

class HR_Timer : public panglos::Timer
{
    timer_group_t group;
    timer_idx_t num;

    virtual void start(bool periodic) override
    {
        esp_err_t err;

        // TODO : other config,  etc.
        err = timer_set_auto_reload(group, num, periodic ? TIMER_AUTORELOAD_EN : TIMER_AUTORELOAD_DIS);
        err = timer_start(group, num);
        CHECK(err);
    }

    virtual void stop() override
    {
        esp_err_t err;
        err = timer_pause(group, num);
        CHECK(err);
    }

    virtual void set_period(Period p) override
    {
        esp_err_t err;
        err = timer_set_alarm_value(group, num, p);
        CHECK(err);
        err = timer_set_alarm(group, num, TIMER_ALARM_EN);
        CHECK(err);
    }

    struct IrqArg
    {
        HR_Timer *timer;
        void (*fn)(Timer *, void *);
        void *arg;
    };

    IrqArg irq;

    static bool irq_handler(void *arg)
    {
        ASSERT(arg);
        struct IrqArg *ia = (struct IrqArg*) arg;
        ASSERT(ia->fn);
        ia->fn(ia->timer, ia->arg);
        return false;
    }
 
    virtual void set_handler(void (*fn)(Timer *, void *), void *arg) override
    {
        esp_err_t err;
 
        if (irq.fn)
        {
            err = timer_isr_callback_remove(group, num);
            CHECK(err);
        }

        irq.fn = fn;
        irq.arg = arg;

        if (fn)
        {
            int intr_alloc_flags = 0;
            err = timer_isr_callback_add(group, num, irq_handler, & irq, intr_alloc_flags);
            CHECK(err);
        }
    }

    virtual Period get() override
    {
        uint64_t val = 0;
        esp_err_t err = timer_get_counter_value(group, num, & val);
        CHECK(err);
        return val;
    }

public:

    HR_Timer(uint32_t _group, uint32_t _num=0)
    {
        ASSERT(_group < TIMER_GROUP_MAX);
        group = (timer_group_t) _group;
        num = (timer_idx_t) _num;
        irq.timer = this;
        irq.fn = 0;
        irq.arg = 0;

        timer_config_t config = {
            .alarm_en=TIMER_ALARM_DIS,
            .counter_en=TIMER_PAUSE,
            .intr_type=TIMER_INTR_LEVEL,
            .counter_dir=TIMER_COUNT_UP,
            .auto_reload=TIMER_AUTORELOAD_DIS,
            .divider=2,
        };
        esp_err_t err;
        err = timer_init(group, num, & config);
        CHECK(err);
    }

    virtual ~HR_Timer()
    {
        // remove / disable irqs
        set_handler(0, 0);
        esp_err_t err;
        err = timer_deinit(group, num);
        CHECK(err);
    }
};

Timer *create_hr_timer(uint32_t group, uint32_t num)
{
    return new HR_Timer(group, num);
}

}   //  namespace panglos

#endif  //  ESP32

//  FIN
