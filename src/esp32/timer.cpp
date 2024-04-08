
#if defined(ESP32)

#include "esp_idf_version.h"
#include "esp_timer.h"

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

class HR_Base : public panglos::Timer
{
protected:
    struct IrqArg
    {
        HR_Base *timer;
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
};

#if (ESP_IDF_VERSION_MAJOR == 4) || ((ESP_IDF_VERSION_MAJOR == 5) && (ESP_IDF_VERSION_MINOR < 1))

    /*
     *  ESP-IDF Version 4 timer API
     */

#include "driver/timer.h"

class HR_Timer : public HR_Base
{
    timer_group_t group;
    timer_idx_t num;

    virtual void start(bool periodic) override
    {
        esp_err_t err;

        err = timer_set_auto_reload(group, num, periodic ? TIMER_AUTORELOAD_EN : TIMER_AUTORELOAD_DIS);
        esp_check(err);
        err = timer_start(group, num);
        esp_check(err);
    }

    virtual void stop() override
    {
        esp_err_t err;
        err = timer_pause(group, num);
        esp_check(err);
    }

    virtual void set_period(Period p) override
    {
        esp_err_t err;
        err = timer_set_alarm_value(group, num, p);
        esp_check(err);
        err = timer_set_alarm(group, num, TIMER_ALARM_EN);
        esp_check(err);
    }

    virtual void set_handler(void (*fn)(Timer *, void *), void *arg) override
    {
        esp_err_t err;
 
        if (irq.fn)
        {
            err = timer_isr_callback_remove(group, num);
            esp_check(err);
        }

        irq.fn = fn;
        irq.arg = arg;

        if (fn)
        {
            int intr_alloc_flags = 0;
            err = timer_isr_callback_add(group, num, irq_handler, & irq, intr_alloc_flags);
            esp_check(err);
        }
    }

    virtual Period get() override
    {
        uint64_t val = 0;
        esp_err_t err = timer_get_counter_value(group, num, & val);
        esp_check(err);
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
        esp_check(err);
    }

    virtual ~HR_Timer()
    {
        // remove / disable irqs
        set_handler(0, 0);
        esp_err_t err;
        err = timer_deinit(group, num);
        esp_check(err);
    }
};

#else

    /*
     *  ESP-IDF Version 5 API
     */

#include "driver/gptimer.h"

class HR_Timer : public HR_Base
{
    gptimer_handle_t handle;
    gptimer_event_callbacks_t callbacks[2];
    gptimer_alarm_config_t alarm_config;
    bool running;

    virtual void start(bool periodic) override
    {
        alarm_config.flags.auto_reload_on_alarm = periodic;
        esp_err_t err;
        err = gptimer_set_alarm_action(handle, & alarm_config);
        esp_check(err);
        err = gptimer_enable(handle);
        esp_check(err);
        running = true;
    }

    virtual void stop() override
    {
        esp_err_t err;
        err = gptimer_disable(handle);
        esp_check(err);
        running = false;
    }

    virtual void set_period(Period p) override
    {
        alarm_config.alarm_count = p;
        alarm_config.reload_count = 0;

        if (running & alarm_config.flags.auto_reload_on_alarm)
        {
            esp_err_t err;
            err = gptimer_set_alarm_action(handle, & alarm_config);
            esp_check(err);
        }
    }

    static bool alarm_cb(gptimer_handle_t, const gptimer_alarm_event_data_t *, void *user_ctx)
    {
        return HR_Base::irq_handler(user_ctx);
    }

    virtual void set_handler(void (*fn)(Timer *, void *), void *arg) override
    {
        esp_err_t err;

        if (irq.fn)
        {
            // Unregister
            err = gptimer_register_event_callbacks(handle, 0, 0);
            esp_check(err);
        }

        irq.fn = fn;
        irq.arg = arg;

        if (fn)
        {
            err = gptimer_register_event_callbacks(handle, callbacks, this);
            esp_check(err);
        }
    }

    virtual Period get() override
    {
        uint64_t value = 0;
        esp_err_t err = gptimer_get_captured_count(handle, & value);
        esp_check(err);
        return value;
    }

public:
    HR_Timer(uint32_t group=0, uint32_t num=0)
    :   running(false)
    {
        irq.fn = 0;
        irq.arg = 0;
        irq.timer = this;
        callbacks[0].on_alarm = alarm_cb;
        callbacks[1].on_alarm = 0;

        gptimer_config_t config = {
            .clk_src=GPTIMER_CLK_SRC_DEFAULT,
            .direction=GPTIMER_COUNT_UP,
            .resolution_hz=80000000,
            .intr_priority=0,
            .flags={ .intr_shared=0 },
        };
        esp_err_t err;
        
        err = gptimer_new_timer(& config, & handle);
        esp_check(err);
    }

    ~HR_Timer()
    {
        set_handler(0, 0);
        esp_err_t err;
        err = gptimer_del_timer(handle);
        esp_check(err);
    }    
};

#endif // (ESP_IDF_VERSION_MAJOR == 4/5)

Timer *create_hr_timer(uint32_t group, uint32_t num)
{
    return new HR_Timer(group, num);
}

}   //  namespace panglos

#endif  //  ESP32

//  FIN
