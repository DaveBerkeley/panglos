
#include <semaphore.h>
#include <string.h>

#include "panglos/debug.h"
#include "panglos/mutex.h"
#include "panglos/semaphore.h"
#include "panglos/event.h"
#include "panglos/time.h"

#include "panglos/drivers/gpio.h"
#include "panglos/drivers/spi.h"
#include "panglos/drivers/i2c.h"

void mock_setup(bool event_thread);
void mock_teardown();
void xxx();

bool pins_match(int num, int start, const int *pins);

void mock_timer_set(panglos::timer_t t);
panglos::timer_t timer_get();

namespace panglos {
    extern EventQueue event_queue;
}

class MockSemaphore : public panglos::Semaphore
{
public:
    bool set;

    virtual ~MockSemaphore(){};

    MockSemaphore() : set(false) { }

    virtual void post() { set = true; }
    virtual void wait() {}
    virtual void set_hook(panglos::PostHook*) {}
};

    /*
     *  GPIO
     */

class MockPin : public panglos::GPIO
{
    int pin;

public:
    enum Irq
    {
        NONE,
        SET,
        CLR,
        CHANGE,
    };
private:

    enum Irq change;

    void (*irq_fn)(void*);
    void *irq_arg;
public:
    MockPin(int pin, enum Irq change=NONE);

    virtual void set(bool state) override;
    virtual bool get() override;
    virtual void toggle() override;
    
    virtual void set_interrupt_handler(enum Interrupt irq, void (*fn)(void*), void *arg) override;
};

class MockSpi : public panglos::SPI
{
public:
    uint8_t buff[64];
    int in;
    uint8_t rd_data[64];

    MockSpi() : SPI(0), in(0) { reset(); }

    virtual bool write(const uint8_t *data, int size)
    {
        ASSERT(data);
        ASSERT((size+in) < (int) sizeof buff);
        memcpy(& buff[in], data, (size_t) size);
        in += size;
        return true;
    }

    virtual bool read(const uint8_t *data, uint8_t *rd, int size)
    {
        ASSERT(data);
        ASSERT(rd);

        ASSERT(size >= 2);

        write(data, size);

        rd[0] = data[0];
        rd[1] = data[1];
        memcpy(& rd[2], rd_data, (size_t) (size-2));

        return true;
    }

    void reset() 
    {
        in = 0;
        memset(buff, 0, sizeof(buff));
        memset(rd_data, 0, sizeof(rd_data));
    }

    void set_read(const uint8_t *data, int size)
    {
        ASSERT(size < (int) sizeof(rd_data));
        memcpy(rd_data, data, (size_t) size);
    }

    void set_read(uint8_t data)
    {
        set_read(& data, 1);
    }
};

    /*
     *
     */

class MockI2C : public panglos::I2C
{
public:
    uint8_t regs[64];

    MockI2C();
    virtual bool probe(uint8_t addr, uint32_t timeout) override;
    virtual int write(uint8_t addr, const uint8_t* wr, uint32_t len) override;
    virtual int write_read(uint8_t addr, const uint8_t* wr, uint32_t len_wr, uint8_t* rd, uint32_t len_rd) override;
    virtual int read(uint8_t addr, uint8_t* rd, uint32_t len) override;
};

    /*
     *  Time / Tick
     */

void time_set(panglos::Time::tick_t t);
bool time_set_auto(bool on);

class TickState
{
    bool was;
public:
    TickState(bool _auto)
    {
        was = time_set_auto(_auto);
    }
    ~TickState()
    {
        time_set_auto(was);
    }
};

//  FIN
