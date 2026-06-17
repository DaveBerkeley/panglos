
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

#if 0
TEST(CliCmd, Test)
{
    TestCli test;
    CLI *cli = test.get_cli();

    add_cli_commands(cli);

    const char *cmd[] = {
        "help\n",
        0,
    };

    test.process(cmd);

    PO_DEBUG("'%s'", test.get());
}
#endif

//  FIN
