
#include <pthread.h>

#include <gtest/gtest.h>

#include "mock.h"

#include "../debug.h"
#include "../esp8266.h"
#include "../esp8266_cmd.h"

using namespace panglos;

class MockRadio : public Radio
{
    const char **ats;
    const char *extra;
public:
    Buffer rd, wr;

    MockRadio(const char** _ats, const char *x="")
    : ats(_ats), extra(x), rd(1024), wr(1024) 
    { }

    virtual void request_command(Command *cmd)
    {
        cmd->start();

        // send the command to the UART
        if (cmd->at)
        {
            write((const uint8_t*) cmd->at, strlen(cmd->at));
        }

        // push the test responses back to the command
        for (const char **s = ats; *s; s++)
        {
            cmd->process((const uint8_t*) *s);
        }
        for (const char *s = extra; *s; s++)
        {
            cmd->consume(*s);
        }
    }

    virtual void end_command(Command *cmd, Semaphore *s)
    {
        //PO_DEBUG("");
        IGNORE(cmd);
        s->post();
    }

    virtual int write(const uint8_t *data, int size)
    {
        //PO_DEBUG("");
        //PO_DEBUG("%s %d", data, size);
        for (int i = 0; i < size; i++)
        {
            wr.add(*data++);
        }
        return size;
    }

    virtual int read(uint8_t *data, int size)  
    {
        //PO_DEBUG("");
        return rd.read(data, size);
    }
};

    /*
     *
     */

TEST(esp8266_CMD, AtOk)
{
    const char *at[] = { "ATxxxx", "OK", 0 };
    MockRadio radio(at);

    AtCommand cmd(& radio, "ATxxxx\r\n", "name");    

    cmd.run();
    EXPECT_EQ(ESP8266::Command::OK, cmd.result);

    uint8_t buff[64];
    int n = radio.wr.read(buff, sizeof(buff));
    buff[n] = '\0';
    EXPECT_STREQ("ATxxxx\r\n", (const char*) buff);
}

TEST(esp8266_CMD, AtError)
{
    const char *at[] = { "ATxxxx", "ERROR", 0 };
    MockRadio radio(at);

    AtCommand cmd(& radio, "ATxxxx\r\n", "name");    

    cmd.run();
    EXPECT_EQ(ESP8266::Command::ERR, cmd.result);

    uint8_t buff[64];
    int n = radio.wr.read(buff, sizeof(buff));
    buff[n] = '\0';
    EXPECT_STREQ("ATxxxx\r\n", (const char*) buff);
}

TEST(esp8266_CMD, AP)
{
    const char *at[] = { "ATxxxx", "WIFI DISCONNECT", "WIFI CONNECTED", "WIFI GOT IP", "OK", 0 };
    MockRadio radio(at);

    ConnectAp cmd(& radio, "ssid", "pw");
    EXPECT_EQ(ESP8266::Command::INIT, cmd.result);

    cmd.run();
    EXPECT_EQ(ESP8266::Command::OK, cmd.result);

    uint8_t buff[64];
    int n = radio.wr.read(buff, sizeof(buff));
    buff[n] = '\0';
    EXPECT_STREQ("AT+CWJAP_DEF=\"ssid\",\"pw\"\r\n", (const char*) buff);
}

TEST(esp8266_CMD, Connect)
{
    const char *at[] = { "ATxxxx", "CONNECT", "OK", 0 };
    MockRadio radio(at);

    Connect cmd(& radio, "hostname", 1234);
    EXPECT_EQ(ESP8266::Command::INIT, cmd.result);

    cmd.run();
    EXPECT_EQ(ESP8266::Command::OK, cmd.result);

    uint8_t buff[64];
    int n = radio.wr.read(buff, sizeof(buff));
    buff[n] = '\0';
    EXPECT_STREQ("AT+CIPSTART=\"TCP\",\"hostname\",1234\r\n", (const char*) buff);
}

TEST(esp8266_CMD, Send)
{
    const char *at[] = { "ATxxxx", "OK", "Recv xx bytes", "SEND OK", 0 };
    MockRadio radio(at, "> ");

    const uint8_t data[] = { 0x01, 0x02, 0x03, 0x04 };
    Send cmd(& radio, data, sizeof(data));
    EXPECT_EQ(ESP8266::Command::INIT, cmd.result);

    cmd.run();
    EXPECT_EQ(ESP8266::Command::OK, cmd.result);

    uint8_t buff[64];
    int n = radio.wr.read(buff, sizeof(buff));
    buff[n] = '\0';
    EXPECT_STREQ("AT+CIPSEND=4\r\n\x1\x2\x3\x4", (const char*) buff);
}

TEST(esp8266_CMD, Read)
{
    const char *at[] = { 0 };
    MockRadio radio(at);

    Semaphore *sem = Semaphore::create();
    uint8_t buff[8];
    int count = 0;
    Read cmd(& radio, sem, buff, sizeof(buff), & count);
    EXPECT_EQ(ESP8266::Command::INIT, cmd.result);

    for (const char *s = "abcdefgh"; *s; s++)
    {
        radio.rd.add(*s);
    }

    radio.request_command(& cmd);
    sem->wait();

    EXPECT_EQ(ESP8266::Command::OK, cmd.result);

    //int n = radio.wr.read(buff, sizeof(buff));
    //buff[n] = '\0';
    //EXPECT_STREQ("AT+CIPSEND=4\r\n\x1\x2\x3\x4", (const char*) buff);
    
    delete sem;
}

//  FIN
