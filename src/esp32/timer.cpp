
#if defined(ESP32)

#include <esp_timer.h>

#include "panglos/debug.h"

#include "panglos/hal.h"

#include "panglos/drivers/timer.h"

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

Timer *Timer::create()
{
    return new ESP_Timer;
}

}   //  namespace panglos

#endif  //  ESP32

//  FIN
