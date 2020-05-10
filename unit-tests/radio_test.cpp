
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

TEST(Radio, Init)
{
    mock_setup(true);

    Out out;
    Mutex *rd_mutex = Mutex::create();
    Semaphore *rd_sem = Semaphore::create();
    Radio::RdBuff *rd = new Radio::RdBuff(1024, rd_sem, rd_mutex);
    MockPin pin(1);
    Radio *radio = new Radio(& out, rd, rd_sem, & pin);

    int err = radio->init();
    EXPECT_EQ(0, err);

    delete radio;
    delete rd;
    delete rd_mutex;
    delete rd_sem;
    mock_teardown();
}

    /*
     *
     */

TEST(Radio, Connect)
{
    mock_setup(true);

    Out out;
    Mutex *rd_mutex = Mutex::create();
    Semaphore *rd_sem = Semaphore::create();
    Radio::RdBuff *rd = new Radio::RdBuff(1024, rd_sem, rd_mutex);
    Radio *radio = new Radio(& out, rd, rd_sem, 0);

    panglos::timer_t timeout = 120000;
    bool okay;

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

    /*
     *
     */

TEST(Radio, SocketOpen)
{
    mock_setup(true);

    Out out;
    Mutex *rd_mutex = Mutex::create();
    Semaphore *rd_sem = Semaphore::create();
    Radio::RdBuff *rd = new Radio::RdBuff(1024, rd_sem, rd_mutex);
    Radio *radio = new Radio(& out, rd, rd_sem, 0);

    panglos::timer_t timeout = 120000;
    bool okay;

    okay = radio->socket_open("hostname", 1234, timeout);
    EXPECT_FALSE(okay);

    char buff[1024];
    int n = out.buffer.read((uint8_t*) buff, sizeof(buff));
    buff[n] = '\0';
    const char *expect = "AT+CIPSTART=\"TCP\",\"hostname\",1234\r\n";
    EXPECT_STREQ(expect, buff);

    // push some data to the rx input
    const char *str[] = {
        expect,
        "CONNECT",
        //"FAIL",
        "OK",
        0
    };

    for (const char **s = str; *s; s++)
    {
        rd->add((const uint8_t*) *s, strlen(*s));
        rd->add((const uint8_t*) "\r\n", 2);
    }

    okay = radio->socket_open("hostname", 1234, timeout);
    EXPECT_TRUE(okay);

    delete radio;
    delete rd;
    delete rd_mutex;
    delete rd_sem;
    mock_teardown();
}

    /*
     *
     */

TEST(Radio, SocketSend)
{
    mock_setup(true);

    Out out;
    Mutex *rd_mutex = Mutex::create();
    Semaphore *rd_sem = Semaphore::create();
    Radio::RdBuff *rd = new Radio::RdBuff(1024, rd_sem, rd_mutex);
    Radio *radio = new Radio(& out, rd, rd_sem, 0);

    panglos::timer_t timeout = 120000;
    int n;

    n = radio->socket_send("abcdefgh", 8, timeout);
    EXPECT_EQ(0, n);

    char buff[1024];
    n = out.buffer.read((uint8_t*) buff, sizeof(buff));
    buff[n] = '\0';
    const char *expect = "AT+CIPSEND=8\r\n";
    EXPECT_STREQ(expect, buff);

    // push some data to the rx input
    const char *str[] = {
        expect,
        "OK",
        //"FAIL",
        0
    };

    // intersperse some rx data
    const char *t = "+IPD,4:wxyz";
    rd->add((const uint8_t*) t, strlen(t));

    for (const char **s = str; *s; s++)
    {
        rd->add((const uint8_t*) *s, strlen(*s));
        rd->add((const uint8_t*) "\r\n", 2);
    }

    rd->add((const uint8_t*) "> ", 2);
    rd->add((const uint8_t*) "SEND OK\r\n", 9);

    n = radio->socket_send("abcdefgh", 8, timeout);
    EXPECT_EQ(8, n);

    // check that the read data was buffered
    n = radio->socket_read(buff, 4, timeout);
    EXPECT_EQ(4, n);
    buff[n] = '\0';
    EXPECT_STREQ("wxyz", buff);

    delete radio;
    delete rd;
    delete rd_mutex;
    delete rd_sem;
    mock_teardown();
}

    /*
     *
     */

TEST(Radio, SocketRead)
{
    mock_setup(true);

    Out out;
    Mutex *rd_mutex = Mutex::create();
    Semaphore *rd_sem = Semaphore::create();
    Radio::RdBuff *rd = new Radio::RdBuff(1024, rd_sem, rd_mutex);
    Radio *radio = new Radio(& out, rd, rd_sem, 0);

    panglos::timer_t timeout = 120000;
    int n;
    char buff[1024];

    n = radio->socket_read(buff, sizeof(buff), timeout);
    EXPECT_EQ(0, n);

    // push some data to the rx input
    const char *t = "+IPD,8:abcdefgh";
    rd->add((const uint8_t*) t, strlen(t));

    n = radio->socket_read(buff, sizeof(buff), timeout);
    EXPECT_EQ(8, n);

    buff[n] = '\0';
    EXPECT_STREQ("abcdefgh", buff);

    // should timeout with no data
    n = radio->socket_read(buff, sizeof(buff), timeout);
    EXPECT_EQ(0, n);

    // push some more ipd data
    t = "+IPD,8:01234567";
    rd->add((const uint8_t*) t, strlen(t));

    n = radio->socket_read(buff, 4, timeout);
    EXPECT_EQ(4, n);
    buff[n] = '\0';
    EXPECT_STREQ("0123", buff);

    n = radio->socket_read(buff, 6, timeout);
    EXPECT_EQ(4, n);
    buff[n] = '\0';
    EXPECT_STREQ("4567", buff);

    delete radio;
    delete rd;
    delete rd_mutex;
    delete rd_sem;
    mock_teardown();
}

    /*
     *
     */

TEST(Radio, SocketSendError)
{
    mock_setup(true);

    Out out;
    Mutex *rd_mutex = Mutex::create();
    Semaphore *rd_sem = Semaphore::create();
    Radio::RdBuff *rd = new Radio::RdBuff(1024, rd_sem, rd_mutex);
    Radio *radio = new Radio(& out, rd, rd_sem, 0);

    panglos::timer_t timeout = 120000;
    int n;

    n = radio->socket_send("abcdefgh", 8, timeout);
    EXPECT_EQ(0, n);

    char buff[1024];
    n = out.buffer.read((uint8_t*) buff, sizeof(buff));
    buff[n] = '\0';
    const char *expect = "AT+CIPSEND=8\r\n";
    EXPECT_STREQ(expect, buff);

    // push some data to the rx input
    const char *str[] = {
        expect,
        "link is not valid",
        "ERROR",
        0
    };

    for (const char **s = str; *s; s++)
    {
        rd->add((const uint8_t*) *s, strlen(*s));
        rd->add((const uint8_t*) "\r\n", 2);
    }

    n = radio->socket_send("abcdefgh", 8, timeout);
    EXPECT_EQ(-1, n);

    delete radio;
    delete rd;
    delete rd_mutex;
    delete rd_sem;
    mock_teardown();
}

//  FIN
