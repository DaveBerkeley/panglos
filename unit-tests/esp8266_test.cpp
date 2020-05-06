
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

void put(RingBuffer<uint8_t> *rb, const char *rx)
{
    rb->add((const uint8_t*) rx, strlen(rx));
}

class Hook : public ESP8266::Hook
{
    RingBuffer<uint8_t> *rb;

    virtual void on_command(ESP8266::Command *cmd)
    {
        PO_DEBUG("%s", cmd->cmd);

        if (!strncmp(cmd->cmd, "AT+CWMODE=1", strlen("AT+CWMODE=1")))
        {
            put(rb, "AT+CWMODE=1\r\nOK\r\n");
            return;
        }
 
        if (!strncmp(cmd->cmd, "AT+CWJAP_DEF", strlen("AT+CWJAP_DEF")))
        {
            put(rb, "AT+CWJAP_DEF=\"ssid\",\"pw\"\r\nWIFI CONNECTED\r\nWIFI GOT IP\r\nOK\r\n");
            return;
        }
        
        // default : "we don't know what to do" case
        put(rb, "OK\r\n");
    }

public:
    Hook(RingBuffer<uint8_t> *_rb)
    : rb(_rb)
    {
    }
};

TEST(esp8266, Test)
{
    mock_setup(true);

    _Output output;
    Semaphore *s = Semaphore::create();
    RingBuffer<uint8_t> *rb = new RingBuffer<uint8_t>(128, s, 0);

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

    bool okay;

    okay = radio.connect("ssid", "pw");
    EXPECT_TRUE(okay);

    // "AT+CIFSR" // query ip/mac addresses
    // "AT+CIPMUX=0" // open a single link
    // "AT+CIPSTART="TCP","192.168.0.12",333" // connect to TCP server
    // "AT+CIPSEND=10" // send 10 bytes of data
    // send "+++" to end sending?
    // "AT+CIPSENDEX=10" // send 10 bytes of data (may include '\0')
    // "AT+CIPMODE" // config transmission mode
    // "AT+CIPRECVDATA"
    // "AT+CIPDNS" // config DNS
    // "AT+CIPDOMAIN" // make DNS request
    // "AT+CWLAP" // list access points
    // "AT+CWQAP" // disconnect from ap
    // "AT+CWDHCP" // enable / disable DHCP
    // "AT+CWAUTOCONN=<enable>" // auto-connect to the ap

    radio.kill();
    err = pthread_join(thread, 0);
    EXPECT_EQ(0, err);

    delete rb;
    delete s;
    mock_teardown();
}

// FIN
