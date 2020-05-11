
#include <semaphore.h>
#include <string.h>

#include <debug.h>
#include <mutex.h>
#include <event.h>
#include <gpio.h>
#include <spi.h>

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

class LinuxSemaphore : public panglos::Semaphore
{
    panglos::PostHook *hook;
public:
    sem_t semaphore;
    int posted;

    LinuxSemaphore();
 
    virtual ~LinuxSemaphore();
    virtual void post();
    virtual void wait();
    virtual void set_hook(panglos::PostHook *hook);
};

    /*
     *  GPIO
     */

class MockPin : public panglos::GPIO
{
    int pin;
public:
    MockPin(int pin);

    virtual void set(bool state);
    virtual bool get();
    virtual void toggle();
    
    virtual void set_interrupt_handler(void (*fn)(void*), void *arg);
    virtual void on_interrupt();
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
        memcpy(& buff[in], data, size);
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
        memcpy(& rd[2], rd_data, size-2);

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
        memcpy(rd_data, data, size);
    }

    void set_read(uint8_t data)
    {
        set_read(& data, 1);
    }
};

//  FIN
