
#include <pthread.h>

#include <gtest/gtest.h>

#include "mock.h"

#include "../debug.h"
#include "../dispatch.h"
#include "../esp8266.h"
#include "../esp8266_cmd.h"

#if 0
using namespace panglos;

class _Output : public Output
{
    virtual int _putc(char c)
    {
        IGNORE(c);
        return 1;
    }
};

static void *runner(void *arg)
{
    ASSERT(arg);
    ESP8266 *radio = (ESP8266*) arg;

    radio->run();

    return 0;
}

void put(RingBuffer<uint8_t> *rb, const char *rx)
{
    rb->add((const uint8_t*) rx, strlen(rx));
}

static bool assume_ok = false;

    // "AT+CIPMUX=0" // open a single link
    // "AT+CIPSEND=10" // send 10 bytes of data
    // "AT+CIPMODE" // config transmission mode
 
class Hook : public ESP8266::Hook
{
    UART::Buffer *rb;

    virtual void on_command(ESP8266::Command *cmd)
    {
        PO_DEBUG("%s", cmd->at);
        const char *t = 0;

        t = "AT+CWMODE=1";
        if (!strncmp(cmd->at, t, strlen(t)))
        {
            put(rb, t);
            put(rb, "\r\nOK\r\n");
            return;
        }
 
        t = "AT+CWJAP_DEF=\"ssid\",\"pw\"";
        if (!strncmp(cmd->at, t, strlen(t)))
        {
            put(rb, t);
            put(rb, "\r\nWIFI DISCONNECTED\r\nWIFI CONNECTED\r\nWIFI GOT IP\r\nOK\r\n");
            return;
        }
 
        t = "AT+CIPSTART=\"TCP\",\"hostname\",1234";
        if (!strncmp(cmd->at, t, strlen(t)))
        {
            put(rb, t);
            put(rb, "\r\nCONNECT\r\nOK\r\n");
            return;
        }
 
        // default : "we don't know what to do" case
        if (assume_ok)
        {
            put(rb, "OK\r\n");
        }
    }

public:
    Hook(UART::Buffer *_rb)
    : rb(_rb)
    {
    }
};

    /*
     *
     */

class Context {
public:
    _Output output;
    Semaphore *s;
    UART::Buffer *rb;
    ESP8266 *radio;
    pthread_t thread;
    Hook *hook;

    Context()
    : s(0), rb(0), radio(0), hook(0)
    {
        s = Semaphore::create();
        rb = new UART::Buffer(128, s, 0);
        radio = new ESP8266(& output, rb, s, 0);
        hook = new Hook(rb);
    }

    ~Context()
    {
        delete rb;
        delete s;
        delete radio;
        delete hook;
    }
};

 void setup(Context *ctx)
{
    mock_setup(true);

    ctx->radio->set_hook(ctx->hook);

    int err = pthread_create(& ctx->thread, 0, runner, ctx->radio);
    EXPECT_EQ(0, err);

    // wait for radio to be running and hooking semaphores ..
    while (!ctx->radio->running())
    {
        usleep(1000);
    }

    assume_ok = true;
    ctx->radio->start();
}

 void teardown(Context *ctx)
{
    sleep(1);

    ctx->radio->kill();
    int err = pthread_join(ctx->thread, 0);
    EXPECT_EQ(0, err);

    mock_teardown();
}

    /*
     *
     */

#if 0
TEST(esp8266, ApConnect)
{
    Context ctx;
    setup(& ctx);

    ESP8266 *radio = ctx.radio;
    bool okay;

    assume_ok = false;
    okay = radio->connect_to_ap("ssid", "pw");
    EXPECT_TRUE(okay);

    // "AT+CIPMUX=0" // open a single link
    // "AT+CIPSTART="TCP","192.168.0.12",333" // connect to TCP server
    // "AT+CIPSEND=10" // send 10 bytes of data
    // "AT+CIPMODE" // config transmission mode
 
    teardown(& ctx);
}
#endif

    /*
     *
     */

TEST(esp8266, Open)
{
    mock_setup(true);

    _Output output;
    Semaphore *s = Semaphore::create();
    UART::Buffer *rb = new UART::Buffer(128, s, 0);

    ESP8266 radio(& output, rb, s, 0);

    Hook hook(rb);
    radio.set_hook(& hook);

    pthread_t thread;
    int err;

    err = pthread_create(& thread, 0, runner, & radio);
    EXPECT_EQ(0, err);

    // wait for radio to be running and hooking semaphores ..
    while (!radio.running())
    {
        usleep(1000);
    }

    assume_ok = true;
    radio.start();

    assume_ok = false;
    int file = radio.connect("hostname", 1234);
    EXPECT_EQ(1, file);

    sleep(1);

    radio.kill();
    err = pthread_join(thread, 0);
    EXPECT_EQ(0, err);

    delete rb;
    delete s;
    mock_teardown();
}
#endif
// FIN
