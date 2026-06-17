
#include "gtest/gtest.h"

#include "cli/src/cli.h"
#include "cli/test/test_io.h"

#include "panglos/debug.h"
#include "panglos/object.h"

#include "panglos/app/cli_cmd.h"

using namespace panglos;

    /*
     *
     */

class TestCli
{
    CLI cli;
    IO io;
public:

    TestCli()
    {
        Objects::objects = Objects::create();
        memset(& cli, 0, sizeof(cli));
        cli.output = io.open();
        cli.prompt = "> ";
        cli.eol = "\r\n";
        cli_init(& cli, 64, 0);
        cli.echo = false;
    }

    ~TestCli()
    {
        cli_close(& cli);
        io.close();
        delete Objects::objects;
        Objects::objects = 0;
    }

    CLI *get_cli()
    {
        return & cli;
    }

    const char *get()
    {
        return io.get();
    }

    void reset()
    {
        io.reset();
    }

    void process(const char *line)
    {
        for (const char *s = line; *s; s++)
        {
            cli_process(& cli, *s);
        }
    }

    void process(const char **lines)
    {
        for (const char **line = lines; *line; line++)
        {
            process(*line);
        }
    }
};

    /*
     *
     */

#if defined(PO_RTC)

#include "panglos/drivers/rtc.h"

class _RTC : public RTC
{
    DateTime dt;

    virtual bool init() override
    {
        return true;
    }
    virtual bool set(const DateTime *_dt) override
    {
        memcpy(& dt, _dt, sizeof(dt));
        return true;
    }
    virtual bool get(DateTime *_dt) override
    {
        memcpy(_dt, & dt, sizeof(dt));
        return true;
    }
public:
    _RTC()
    {
        memset(& dt, 0, sizeof(dt));
    }
};

TEST(CliCmd, RTC)
{
    TestCli test;
    CLI *cli = test.get_cli();

    RTC *rtc = new _RTC;
    Objects::objects->add("rtc", rtc);

    add_cli_commands(cli);

    {
        test.process("rtc get\n");
        const char *s = test.get();
        EXPECT_TRUE(strstr(s, "0000-00-00 00:00:00"));
        test.reset();
    }

    {
        struct Pair {
            const char *data;
            const char *expects;
        };

        const char *parse = "error parsing";
        const char *invalid = "Invalid date-time";
        const char *reading = "error reading";
        const struct Pair pairs[] = {
            {   "rtc set a b c d e f\n", parse },
            {   "rtc set 0 0 0 0 0 f\n", parse },
            {   "rtc set 0 0 0 \n", reading },
            {   "rtc set\n", reading },
            {   "rtc set 0 0 0 0 0 0\n", invalid },
            {   "rtc set 2026 13 1 0 0 0\n", invalid },
            {   "rtc set 2026 4 31 0 0 0\n", invalid },
            {   "rtc set 1999 4 3 0 0 0\n", invalid },
            {   "rtc set 2026 4 3 24 0 0\n", invalid },
            {   "rtc set 2026 4 3 10 60 0\n", invalid },
            {   "rtc set 2026 4 3 10 6 60\n", invalid },
            { 0 },
        };

        for (const struct Pair *p = pairs; p->data; p++)
        {
            test.process(p->data);
            const char *s = test.get();
            EXPECT_TRUE(strstr(s, p->expects));
            //PO_DEBUG("%s '%s'", p->data, s);
            test.reset();
        }
    }

    {
        const char *cmd[] = {
            "rtc set 2026 06 16 20 11 15\n", 
            "rtc get\n",
            0,
        };

        test.process(cmd);
        const char *s = test.get();
        EXPECT_TRUE(strstr(s, "2026-06-16 20:11:15"));
        //PO_DEBUG("'%s'", s);
        test.reset();
    }

    delete rtc;
}

#endif  //  PO_RTC

    /*
     *
     */

#if defined(PO_I2C)

#include "panglos/drivers/i2c.h"

class _I2C : public I2C
{
    virtual bool probe(uint8_t addr, uint32_t ) override
    {
        return (addr == 0x12) || (addr == 0x35);
    }

    virtual int write(uint8_t , const uint8_t* , uint32_t ) override
    {
        return 0;
    }

    virtual int write_read(uint8_t , const uint8_t* , uint32_t , uint8_t* , uint32_t ) override
    {
        return 0;
    }

    virtual int read(uint8_t , uint8_t* , uint32_t ) override
    {
        return 0;
    }
public:
    _I2C()
    :   I2C(0)
    {
    }
};

TEST(CliCmd, I2C)
{
    TestCli test;
    CLI *cli = test.get_cli();

    I2C *i2c = new _I2C;
    Objects::objects->add("i2c", i2c);

    add_cli_commands(cli);

    { 
        // probe test : fail
        const uint8_t bad[] = { 0, 0x01, 0x11, 0x13, 0x7f, 0x80 };
        for (size_t idx = 0; idx < sizeof(bad); idx++)
        {
            char cmd[24];
            snprintf(cmd, sizeof(cmd), "i2c %#x\n", bad[idx]);
            test.process(cmd);
            const char *s = test.get();
            EXPECT_TRUE(strstr(s, "no device found"));
            //PO_DEBUG("'%s' '%s'", cmd, s);
            test.reset();
        }
    }

    { 
        // probe test : pass
        const uint8_t good[] = { 0x12, 0x35 };
        for (size_t idx = 0; idx < sizeof(good); idx++)
        {
            char cmd[24];
            snprintf(cmd, sizeof(cmd), "i2c %#x\n", good[idx]);
            test.process(cmd);
            const char *s = test.get();
            EXPECT_TRUE(strstr(s, "found"));
            //PO_DEBUG("'%s' '%s'", cmd, s);
            test.reset();
        }
    }

    { 
        // scan test
        test.process("i2c\n");
        const char *s = test.get();
        EXPECT_TRUE(strstr(s, "found 0x12"));
        EXPECT_TRUE(strstr(s, "found 0x35"));
        //PO_DEBUG("'%s'", s);
        test.reset();
    }

    delete i2c;
}

#endif  //  PO_I2C

    /*
     *
     */

#include "panglos/drivers/gpio.h"
#include "panglos/mutex.h"

class _GPIO : public GPIO
{
    panglos::Mutex *mutex;
    bool state;
    int count;

    virtual void set(bool s) override
    {
        panglos::Lock lock(mutex);
        state = s;
    }

    virtual bool get() override
    {
        panglos::Lock lock(mutex);
        return state;
    }

    virtual void toggle() override
    {
        panglos::Lock lock(mutex);
        state = !state;
        count += 1;
    }
public:
    _GPIO()
    :   mutex(0),
        state(false),
        count(0)
    {
        mutex = panglos::Mutex::create();
    }
    ~_GPIO()
    {
        delete mutex;
    }

    int get_count() { return count; }
};

#include "panglos/thread.h"
#include "panglos/time.h"

class TestDelay
{
    Thread *thread;
    GPIO *gpio;
    int period; // ms

public:

    TestDelay(GPIO *g, int ms)
    :   thread(0),
        gpio(g),
        period(ms)
    {
        thread = Thread::create("gpio");
        thread->start(delay, this);
    }

    ~TestDelay()
    {
        thread->join();
        delete thread;
    }

    void delay()
    {
        Time::msleep(period);
        if (gpio) gpio->toggle();
        Time::msleep(period);
        gpio_terminate_loop();
    }

    static void delay(void *arg)
    {
        ASSERT(arg);
        TestDelay *obj = (TestDelay *) arg;
        obj->delay();
    }
};

TEST(CliCmd, GPIO)
{
    TestCli test;
    CLI *cli = test.get_cli();

    add_cli_commands(cli);

    _GPIO *led = new _GPIO;
    Objects::objects->add("led", led);

    { 
        // no gpio
        test.process("gpio\n");
        const char *s = test.get();
        EXPECT_TRUE(strstr(s, "expects name of gpio"));
        //PO_DEBUG("'%s'", s);
        test.reset();
    }

    { 
        // no cmd
        test.process("gpio led\n");
        const char *s = test.get();
        EXPECT_TRUE(strstr(s, "expects state"));
        //PO_DEBUG("'%s'", s);
        test.reset();
    }

    { 
        // invalid gpio
        test.process("gpio xxx\n");
        const char *s = test.get();
        EXPECT_TRUE(strstr(s, "can't find device"));
        //PO_DEBUG("'%s'", s);
        test.reset();
    }

    { 
        // query state
        test.process("gpio led ?\n");
        const char *s = test.get();
        EXPECT_TRUE(strstr(s, "0"));
        EXPECT_FALSE(strstr(s, "1"));
        //PO_DEBUG("'%s'", s);
        test.reset();
    }

    { 
        // set state
        test.process("gpio led 1\n");
        test.reset();
        test.process("gpio led ?\n");
        const char *s = test.get();
        EXPECT_TRUE(strstr(s, "1"));
        EXPECT_FALSE(strstr(s, "0"));
        //PO_DEBUG("'%s'", s);
        test.reset();
    }

    { 
        // toggle state
        test.process("gpio led 1\n");
        test.reset();
        test.process("gpio toggle led\n");
        test.reset();
        test.process("gpio led ?\n");
        const char *s = test.get();
        EXPECT_TRUE(strstr(s, "0"));
        EXPECT_FALSE(strstr(s, "1"));
        //PO_DEBUG("'%s'", s);
        test.reset();
    }

    { 
        // show state
        // check that the led toggles from 0 to 1
        TestDelay delay(led, 50);

        test.process("gpio led 0\n");
        test.reset();
        test.process("gpio show 10 led\n");
        const char *s = test.get();
        const char *m = strstr(s, "led 0");
        EXPECT_TRUE(m);
        EXPECT_TRUE(strstr(m, "led 1"));
        //PO_DEBUG("'%s'", s);
        test.reset();
    }

    { 
        // flash state
        // count the flashes
        TestDelay delay(0, 50);
        const int start = led->get_count();
        test.process("gpio flash 10 led\n");
        const int end = led->get_count();
        EXPECT_LT(start, end);
        test.reset();
    }

    delete led;
}

//  FIN
