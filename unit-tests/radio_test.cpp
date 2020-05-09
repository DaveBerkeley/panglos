
#include <pthread.h>

#include <gtest/gtest.h>

#include "../debug.h"
#include "../radio.h"
#include "mock.h"

using namespace panglos;

class Out : public Output
{
public:
    Buffer buffer;

    Out() : buffer(1024) { }

    virtual int _putc(char c)
    {
        return buffer.add(c);
    }
};  

    /*
     *
     */

TEST(Radio, Test)
{
    mock_setup(true);

    Out out;
    Mutex *rd_mutex = Mutex::create();
    Semaphore *rd_sem = Semaphore::create();
    Radio::RdBuff *rd = new Radio::RdBuff(1024, rd_sem, rd_mutex);
    Radio *radio = new Radio(& out, rd, rd_sem);

    panglos::timer_t timeout = 120000;
    bool okay;

    // timeout
    okay = radio->connect("ssid", "pw", timeout);
    EXPECT_FALSE(okay);

    char buff[1024];
    int n = out.buffer.read((uint8_t*) buff, sizeof(buff));
    buff[n] = '\0';
    const char *expect = "AT+CWJAP_DEF=\"ssid\",\"pw\"\r\n";
    EXPECT_STREQ(expect, buff);

    // push some data to the rx input
    const char *str[] = {
        expect,
        "WIFI DISCONNECT",
        "WIFI CONNECTED",
        "WIFI GOT IP",
        //"FAIL",
        "OK",
        0
    };

    for (const char **s = str; *s; s++)
    {
        rd->add((const uint8_t*) *s, strlen(*s));
        rd->add((const uint8_t*) "\r\n", 2);
    }

    okay = radio->connect("ssid", "pw", timeout);
    EXPECT_TRUE(okay);

    delete radio;
    delete rd;
    delete rd_mutex;
    delete rd_sem;
    mock_teardown();
}

//  FIN
