
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
};

    /*
     *
     */

TEST(CliCmd, Test)
{
    TestCli test;
    CLI *cli = test.get_cli();

    add_cli_commands(cli);

    const char *cmd[] = {
        "help\n",
        0,
    };

    for (const char **line = cmd; *line; line++)
    {
        test.process(*line);
    }

    PO_DEBUG("'%s'", test.get());
}

//  FIN
