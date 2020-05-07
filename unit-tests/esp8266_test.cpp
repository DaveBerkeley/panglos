
#include <pthread.h>

#include <gtest/gtest.h>

#include "mock.h"

#include "../debug.h"
#include "../dispatch.h"
#include "../esp8266.h"
#include "../esp8266_cmd.h"

using namespace panglos;

class _Output : public Output
{
public:
    Buffer buffer;

    _Output() : buffer(1024) { }

    virtual int _putc(char c)
    {
        return buffer.add(c);
    }
};

static void *runner(void *arg)
{
    ASSERT(arg);
    ESP8266 *radio = (ESP8266*) arg;

    radio->run();

    return 0;
}

void put(UART::Buffer *rb, const char *rx)
{
    rb->add((const uint8_t*) rx, strlen(rx));
}

class Hook : public ESP8266::Hook
{
    UART::Buffer *rb;

    virtual void on_command(ESP8266::Command *cmd)
    {
        PO_DEBUG("%s", cmd->at);

        const char *t = "AT+CWMODE=1";
        if (!strncmp(cmd->at, t, strlen(t)))
        {
            put(rb, "AT+CWMODE=1\r\nOK\r\n");
            return;
        }
 
        t = "AT+CWJAP_DEF";
        if (!strncmp(cmd->at, t, strlen(t)))
        {
            put(rb, "AT+CWJAP_DEF=\"ssid\",\"pw\"\r\nWIFI CONNECTED\r\nWIFI GOT IP\r\nOK\r\n");
            return;
        }
 
        t = "AT+CIPSTART=\"TCP\",\"hostname\",1234";
        if (!strncmp(cmd->at, t, strlen(t)))
        {
            put(rb, "AT+CIPSTART=\"TCP\",\"hostname\",1234\r\nCONNECT\r\nOK\r\n");
            return;
        }

        t = "AT+CIPSEND=4";
        if (!strncmp(cmd->at, t, strlen(t)))
        {
            put(rb, "AT+CIPSEND=4\r\nOK\r\n> Recv 4 bytes\r\nSEND OK\r\n");
            return;
        }

        // default : "we don't know what to do" case
        put(rb, "OK\r\n");
    }

public:
    Hook(UART::Buffer *_rb)
    : rb(_rb)
    {
    }
};

TEST(esp8266, Test)
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

    bool okay;
    char buff[64];
    int n;

    //  Start

    radio.start();

    n = output.buffer.read((uint8_t*) buff, sizeof(buff));
    buff[n] = '\0';
    EXPECT_STREQ("AT\r\n", buff);

    // AP Connect

    okay = radio.connect_to_ap("ssid", "pw");
    EXPECT_TRUE(okay);

    n = output.buffer.read((uint8_t*) buff, sizeof(buff));
    buff[n] = '\0';
    EXPECT_STREQ("AT+CWMODE=1\r\nAT+CWJAP_DEF=\"ssid\",\"pw\"\r\n", buff);

    //  Socket connect

    int file = radio.connect("hostname", 1234);
    EXPECT_EQ(1, file);

    n = output.buffer.read((uint8_t*) buff, sizeof(buff));
    buff[n] = '\0';
    EXPECT_STREQ("AT+CIPSTART=\"TCP\",\"hostname\",1234\r\n", buff);

    //  data send

    const uint8_t data[] = { 'a', 'b', 'c', 'd' };
    n = radio.socket_send(0, data, sizeof(data));
    EXPECT_EQ(4, n);

    n = output.buffer.read((uint8_t*) buff, sizeof(buff));
    buff[n] = '\0';
    EXPECT_STREQ("AT+CIPSEND=4\r\nabcd", buff);

    //  data read

    // "AT+CIFSR" // query ip/mac addresses
    // "AT+CIPMUX=0" // open a single link
    // "AT+CIPSEND=10" // send 10 bytes of data
    // "AT+CIPSENDEX=10" // send 10 bytes of data (may include '\0')
    // "AT+CIPMODE" // config transmission mode
    // "AT+CIPRECVDATA"
    // "AT+CIPDNS" // config DNS
    // "AT+CIPDOMAIN" // make DNS request
    // "AT+CWLAP" // list access points
    // "AT+CWQAP" // disconnect from ap
    // "AT+CWDHCP" // enable / disable DHCP

    radio.kill();
    err = pthread_join(thread, 0);
    EXPECT_EQ(0, err);

    delete rb;
    delete s;
    mock_teardown();
}

// FIN
