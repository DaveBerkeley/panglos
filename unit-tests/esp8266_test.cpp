
#include <pthread.h>

#include <gtest/gtest.h>

#include "mock.h"

#include "../debug.h"
#include "../dispatch.h"
#include "../esp8266.h"

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

void put(RingBuffer *rb, const char *rx)
{
    rb->add((const uint8_t*) rx, strlen(rx));
}

class Hook : public ESP8266::Hook
{
    RingBuffer *rb;

    virtual void on_command(ESP8266::Command *cmd)
    {
        PO_DEBUG("%s", cmd->cmd);

        if (!strncmp(cmd->cmd, "AT+CWMODE=1", strlen("AT+CWMODE=1")))
        {
            put(rb, "AT+CWMODE=1\r\nOK\r\n");
        }
 
        if (!strncmp(cmd->cmd, "AT+CWJAP_DEF", strlen("AT+CWJAP_DEF")))
        {
            put(rb, "AT+CWJAP_DEF=\"ssid\",\"pw\"\r\nWIFI CONNECTED\r\nWIFI GOT IP\r\nOK\r\n");
        }
    }

public:
    Hook(RingBuffer *_rb)
    : rb(_rb)
    {
    }
};

TEST(esp8266, Test)
{
    mock_setup(true);

    _Output output;
    Semaphore *s = Semaphore::create();
    RingBuffer *rb = new RingBuffer(128, s);

    ESP8266 radio(& output, rb, s, 0);

    Hook hook(rb);
    radio.set_hook(& hook);

    pthread_t thread;
    int err;

    err = pthread_create(& thread, 0, runner, & radio);
    EXPECT_EQ(0, err);

    sleep(1);

    radio.connect("ssid", "pw");

    sleep(2);
    radio.kill();

    err = pthread_join(thread, 0);
    EXPECT_EQ(0, err);

    delete rb;
    delete s;
    mock_teardown();
}

// FIN
