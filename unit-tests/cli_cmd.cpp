
#include "gtest/gtest.h"

#include "cli/src/cli.h"
#include "cli/test/test_io.h"

#include "panglos/debug.h"
#include "panglos/object.h"
#include "panglos/thread.h"
#include "panglos/time.h"
#include "panglos/mutex.h"
#include "panglos/storage.h"

#include "panglos/app/cli_cmd.h"
#include "panglos/app/cli_gpio.h"

#include "panglos/drivers/rtc.h"
#include "panglos/drivers/i2c.h"
#include "panglos/drivers/gpio.h"

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

#include "panglos/app/cli_rtc.h"

static RTC *rtc_lookup(const char *name, void*)
{
    return (RTC*) Objects::objects->get(name);
}

TEST(CliCmd, RTC)
{
    TestCli test;
    CLI *cli = test.get_cli();

    RTC *rtc = new _RTC;
    Objects::objects->add("rtc", rtc);

    add_cli_rtc_cmd(cli, rtc_lookup, 0);

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

class _I2C : public I2C
{
    virtual bool probe(uint8_t addr, uint32_t ) override
    {
        return (addr == 0x12) || (addr == 0x35);
    }

    virtual int write(uint8_t addr, const uint8_t *wr, uint32_t len) override
    {
        if (addr != 0x12) return 0; // test writes to this device
        if (len == 2)
        {
            EXPECT_EQ(wr[0], 2);
            EXPECT_EQ(wr[1], 4);
            return int(len);
        }
        if (len == 5)
        {
            EXPECT_EQ(wr[0], 16);
            EXPECT_EQ(wr[1], 1);
            EXPECT_EQ(wr[2], 2);
            EXPECT_EQ(wr[3], 3);
            EXPECT_EQ(wr[4], 4);
            return int(len);
        }
        ASSERT(0);
        return int(len);
    }

    virtual int write_read(uint8_t addr, const uint8_t *wr, uint32_t wr_len, uint8_t *rd, uint32_t rd_len) override
    {
        if (!addr) return 0;
        if (addr != 0x60) return 0;
        // rd tests

        static const uint8_t test_data[16] = {
            0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
            0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47,
        };

        ASSERT(wr_len == 1);
        ASSERT(rd_len >= 1);
        ASSERT(wr);
        ASSERT(rd);
        uint8_t reg = *wr;
        ASSERT((reg + rd_len) < sizeof(test_data));

        //PO_DEBUG("addr=%d wr=%p wr_len=%d rd=%p rd_len=%d", addr, wr, wr_len, rd, rd_len);
        memcpy(rd, & test_data[reg], rd_len);
        return int(rd_len);
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

#include "panglos/app/cli_i2c.h"

static I2C *i2c_lookup(const char *name, void *)
{
    static _I2C i2c;
    if (!name) return & i2c;
    if (!strcmp(name, "i2c")) return & i2c;
    return 0;
}

TEST(CliCmd, I2C)
{
    TestCli test;
    CLI *cli = test.get_cli();

    add_cli_i2c_cmd(cli, i2c_lookup, 0);

    struct Test {
        const char *cmd;
        const char *expect[4];
    };

    const struct Test tests[] = {
        {   "i2c scan\n", { "found 0x12", "found 0x35" } },
        {   "i2c scan i2c\n", { "found 0x12", "found 0x35" } },
        {   "i2c scan 100\n", { "no probe response at 0x64" } },
        {   "i2c scan i2c 100\n", { "no probe response at 0x64" } },
        {   "i2c scan 0x12\n", { "found 0x12" } },
        {   "i2c scan i2c 0x12\n", { "found 0x12" } },
        {   "i2c scan xxx\n", { "device 'xxx' not found" } },

        {   "i2c rd\n", { "no device address" } },
        {   "i2c rd i2c\n", { "no device address" } },
        {   "i2c rd xxx\n", { "device 'xxx' not found" } },
        {   "i2c rd 0\n", { "no device address" } },
        {   "i2c rd 0x60\n", { "no reg address" } },
        {   "i2c rd 0x60 0x0\n", { "dev=0x60 0x00: 0x10" } },
        {   "i2c rd 0x60 2\n", { "dev=0x60 0x02: 0x12" } },
        {   "i2c rd 0x60 2 4\n", { "dev=0x60 0x02: 0x12 0x13 0x14 0x15" } },
        {   "i2c rd 0x12 2 4\n", { "error reading device 0x12 at address 0x02" } },

        {   "i2c wr\n", { "no device address" } },
        {   "i2c wr i2c\n", { "no device address" } },
        {   "i2c wr xxx\n", { "device 'xxx' not found" } },
        {   "i2c wr 0\n", { "no device address" } },
        {   "i2c wr 0x60\n", { "no reg address" } },
        {   "i2c wr 0x12 2 4\n", { "written, 2 bytes" } },
        {   "i2c wr 0x12 16 1 2 3 4\n", { "written, 5 bytes" } },
        {   "i2c wr 0x12 16 1 nowt 3 4\n", { "bad value 'nowt'" } },
        { 0 },
    };

    for (const struct Test *t = tests; t->cmd; t++)
    {
        //PO_DEBUG("cli='%s'", t->cmd);
        test.process(t->cmd);
        const char *s = test.get();
        //PO_DEBUG("%s", s);
        for (const char * const *ex = & t->expect[0]; *ex; ex++)
        {
            //PO_DEBUG("ex: %s", *ex);
            EXPECT_TRUE(strstr(s, *ex));
        }
        test.reset();
    }
}

#endif  //  PO_I2C

    /*
     *
     */

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

    //add_cli_gpio_cmd(cli, 0, 0);
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

    /*
     *
     */

TEST(CliCmd, DbList)
{
    TestCli test;
    CLI *cli = test.get_cli();

    add_cli_commands(cli);

    Storage db("test");

    db.set("str", "hello");
    db.set("i8", int8_t(0x12));
    db.set("i16", int16_t(0x1234));
    db.set("i32", int32_t(0x12345678));

    test.process("db list\n");
 
    const char *s = test.get();
    char *buff = (char*) malloc(strlen(s)+1);
    strcpy(buff, s);
    //PO_DEBUG("'%s'", buff);

    char *tok = buff;
    // skip leading prompt
    for (; (*tok == ' ') || (*tok == '>'); tok++)
        ;
    char *save = 0;
    char *start = tok;

    struct Match {
        const char *text;
        bool found;
    };

    struct Match matches[] = {
        {   "test i8 INT8\r", },
        {   "test i16 INT16\r", },
        {   "test i32 INT32\r", },
        {   "test str STR\r", },
        { 0 },
    };

    while (true)
    {
        tok = strtok_r(start, "\n", & save);
        if (!tok) break;
        //PO_DEBUG("x '%s'", tok);
        start = 0;
        for (struct Match *m = matches; m->text; m++)
        {
            if (!strcmp(m->text, tok)) m->found = true;
        }
    }

    for (struct Match *m = matches; m->text; m++)
    {
        EXPECT_TRUE(m->found);
    }
    
    free(buff);
}

    /*
     *
     */

static int get_value(TestCli &test)
{
    const char *s = test.get();

    if (strstr(s, "Error reading value")) return -1;

    char *end = 0;
    long int value = strtol(s, & end, 0);
    test.reset();
    return (int) value;
}

TEST(CliCmd, DbSetGetDel)
{
    TestCli test;
    CLI *cli = test.get_cli();

    add_cli_commands(cli);

    // not set yet
    test.process("db get test value\n");
    EXPECT_EQ(get_value(test), -1);
 
    // set the value
    test.process("db set test value 1234\n");
    test.reset();

    // get the value
    test.process("db get test value\n");
    EXPECT_EQ(get_value(test), 1234);

    // del the value
    test.process("db del test value\n");
    test.reset();
 
    test.process("db get test value\n");
    EXPECT_EQ(get_value(test), -1);
}

TEST(CliCmd, DbBlob)
{
    bool ok;
    TestCli test;
    CLI *cli = test.get_cli();

    add_cli_commands(cli);

    const char *data = "hello world\nlast line\n";

    // set
    test.process("db blob set test value\n");
    test.process(data);
    test.process("\n"); // blob entry is terminated by "\n\n"
    test.reset();

    {
        // check by getting blob directly from the Storage
        Storage db("test");

        size_t size = 0;
        ok = db.get_blob("value", 0, & size);
        EXPECT_TRUE(ok); 
        EXPECT_EQ(size, strlen(data));

        char *buff = (char *) malloc(size + 1);
        buff[size] = '\0'; // terminate the returned string

        ok = db.get_blob("value", buff, & size);
        EXPECT_TRUE(ok);     
        EXPECT_STREQ(data, buff);

        free(buff);
    }

    // check the blob get command
    // get blob returns a hexdump of the string
    test.process("db blob get test value\n");
    const char *s = test.get();

    // copy the cli prints
    char *buff = (char *) malloc(strlen(s) + 1);
    strcpy(buff, s);

    const char *delim = "\r\n ";
    char *end = 0;
    char *tok = strtok_r(buff, delim, & end);

    int compares = 0;
    // iterate through the expected data, checking for addr: and hex data
    for (int idx = 0; tok; idx++)
    {
        char cmp[16];
        if (!(idx % 16))
        {
            // check for XXXX: address
            snprintf(cmp, sizeof(cmp), "%04x:", idx);
            //PO_DEBUG("'%s' - '%s'", cmp, tok);
            EXPECT_STREQ(cmp, tok);
            tok = strtok_r(0, delim, & end);
        }
        if (!tok) break;
        if (strchr(tok, '>')) break; // cursor

        // check for XX data
        snprintf(cmp, sizeof(cmp), "%02x", data[idx]);
        //PO_DEBUG("'%s' - '%s'", cmp, tok);
        EXPECT_STREQ(cmp, tok);
        compares += 1;
        tok = strtok_r(0, delim, & end);
    }

    EXPECT_EQ(compares, strlen(data));

    free(buff);
}

//  FIN
