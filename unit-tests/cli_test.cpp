
#include <stdlib.h>

#include <gtest/gtest.h>

#include <panglos/debug.h>
#include <panglos/cli.h>

#include "mock.h"

using namespace panglos;

typedef struct {
    Action *action;
    int values[CLI::MAX_VALUES];
    int argc;
    void (*fn)(Action *a, int argc, int *argv);
}   CliArg;

static void on_x(Action *a, int argc, int *argv, void (*fn)(Action *a, int argc, int *argv))
{
    ASSERT(a);
    CliArg *cli_arg = (CliArg*) a->arg;

    cli_arg->action = a;
    cli_arg->argc = argc;

    for (int i = 0; i < argc; i++)
    {
        cli_arg->values[i] = argv[i];
    }
    cli_arg->fn = fn;
}

static void on_a(Action *a, int argc, int *argv)
{
    on_x(a, argc, argv, on_a);
}

static void on_b(Action *a, int argc, int *argv)
{
    on_x(a, argc, argv, on_b);
}

static void on_str(Action *a, int argc, int *argv)
{
    on_x(a, argc, argv, on_str);
}

    /*
     *
     */

TEST(Cli, Cli)
{
    CLI cli;
    CliArg arg;

    Action a = { "A", on_a, & arg, 0 };
    cli.add_action(& a);

    // check A
    memset(& arg, 0, sizeof(arg));
    cli.process("A1234\r\n");
    EXPECT_EQ(arg.action, & a);
    EXPECT_EQ(arg.values[0], 1234);
    EXPECT_EQ(arg.fn, on_a);

    Action b = { "B", on_b, & arg, 0 };
    cli.add_action(& b);

    // check B
    memset(& arg, 0, sizeof(arg));
    cli.process("B0\r\n");
    EXPECT_EQ(arg.action, & b);
    EXPECT_EQ(arg.argc, 1);
    EXPECT_EQ(arg.values[0], 0);
    EXPECT_EQ(arg.fn, on_b);

    // check A still works
    memset(& arg, 0, sizeof(arg));
    cli.process("A1234\r\n");
    EXPECT_EQ(arg.action, & a);
    EXPECT_EQ(arg.argc, 1);
    EXPECT_EQ(arg.values[0], 1234);
    EXPECT_EQ(arg.fn, on_a);

    // check some non commands
    memset(& arg, 0, sizeof(arg));
    cli.process("123\r\n");
    EXPECT_EQ(arg.action, (void*)0);
    EXPECT_EQ(arg.argc, 0);
    EXPECT_EQ(arg.values[0], 0);
    EXPECT_EQ(arg.fn, (void*) 0);

    // check no command
    memset(& arg, 0, sizeof(arg));
    cli.process("\r\n");
    EXPECT_EQ(arg.action, (void*)0);
    EXPECT_EQ(arg.argc, 0);
    EXPECT_EQ(arg.fn, (void*) 0);

    // check default 0
    memset(& arg, 0, sizeof(arg));
    cli.process("A\r\n");
    EXPECT_EQ(arg.action, & a);
    EXPECT_EQ(arg.argc, 0);
    EXPECT_EQ(arg.values[0], 0);
    EXPECT_EQ(arg.fn, on_a);

    // check <backspace> deletes (reset)
    // actually, any unknown char will reset the line
    memset(& arg, 0, sizeof(arg));
    cli.process("A\b\r\n");
    EXPECT_EQ(arg.action, (void*)0);
    EXPECT_EQ(arg.argc, 0);
    EXPECT_EQ(arg.fn, (void*)0);
}

TEST(Cli, CliMulti)
{
    CLI cli(":");
    CliArg arg;

    Action a = { "A", on_a, & arg, 0 };
    cli.add_action(& a);

    // check A
    memset(& arg, 0, sizeof(arg));
    cli.process("A1234:1:2:3\r\n");
    EXPECT_EQ(arg.action, & a);
    EXPECT_EQ(arg.argc, 4);
    EXPECT_EQ(arg.values[0], 1234);
    EXPECT_EQ(arg.values[1], 1);
    EXPECT_EQ(arg.values[2], 2);
    EXPECT_EQ(arg.values[3], 3);

    cli.process("A0\r\n");
    EXPECT_EQ(arg.action, & a);
    EXPECT_EQ(arg.argc, 1);
    EXPECT_EQ(arg.values[0], 0);

    cli.process("A0:123:234\r\n");
    EXPECT_EQ(arg.action, & a);
    EXPECT_EQ(arg.argc, 3);
    EXPECT_EQ(arg.values[0], 0);
    EXPECT_EQ(arg.values[1], 123);
    EXPECT_EQ(arg.values[2], 234);
}

TEST(Cli, CliSign)
{
    CLI cli(":");
    CliArg arg;

    Action a = { "A", on_a, & arg, 0 };
    cli.add_action(& a);

    // check A
    memset(& arg, 0, sizeof(arg));
    cli.process("A-1234:1:-2:3\r\n");
    EXPECT_EQ(arg.action, & a);
    EXPECT_EQ(arg.argc, 4);
    EXPECT_EQ(arg.values[0], -1234);
    EXPECT_EQ(arg.values[1], 1);
    EXPECT_EQ(arg.values[2], -2);
    EXPECT_EQ(arg.values[3], 3);

    memset(& arg, 0, sizeof(arg));
    cli.process("A0:123:-234\r\n");
    EXPECT_EQ(arg.action, & a);
    EXPECT_EQ(arg.argc, 3);
    EXPECT_EQ(arg.values[0], 0);
    EXPECT_EQ(arg.values[1], 123);
    EXPECT_EQ(arg.values[2], -234);

    memset(& arg, 0, sizeof(arg));
    cli.process("A-1234:+1:+2:-3\r\n");
    EXPECT_EQ(arg.action, & a);
    EXPECT_EQ(arg.argc, 4);
    EXPECT_EQ(arg.values[0], -1234);
    EXPECT_EQ(arg.values[1], 1);
    EXPECT_EQ(arg.values[2], 2);
    EXPECT_EQ(arg.values[3], -3);

    // implicit seperator after comand
    memset(& arg, 0, sizeof(arg));
    cli.process("A:1:2:-3\r\n");
    EXPECT_EQ(arg.action, & a);
    EXPECT_EQ(arg.argc, 3);
    EXPECT_EQ(arg.values[0], 1);
    EXPECT_EQ(arg.values[1], 2);
    EXPECT_EQ(arg.values[2], -3);

    // '+' can also be a command
    Action b = { "+", on_a, & arg, 0 };
    cli.add_action(& b);

    memset(& arg, 0, sizeof(arg));
    cli.process("+100\r\n");
    EXPECT_EQ(arg.action, & b);
    EXPECT_EQ(arg.argc, 1);
    EXPECT_EQ(arg.values[0], 100);

    memset(& arg, 0, sizeof(arg));
    cli.process("+-100\r\n");
    EXPECT_EQ(arg.action, & b);
    EXPECT_EQ(arg.argc, 1);
    EXPECT_EQ(arg.values[0], -100);
}

TEST(Cli, CliString)
{
    CLI cli;
    CliArg arg;

    Action a = { "GO ", on_str, & arg, 0 };
    Action b = { "MOVE", on_str, & arg, 0 };
    Action c = { "HELP", on_str, & arg, 0 };
    cli.add_action(& a);
    cli.add_action(& b);
    cli.add_action(& c);

    // check GO
    memset(& arg, 0, sizeof(arg));
    cli.process("GO 1234,2345\r\n");
    EXPECT_EQ(arg.action, & a);
    EXPECT_EQ(arg.argc, 2);
    EXPECT_EQ(arg.values[0], 1234);
    EXPECT_EQ(arg.values[1], 2345);

    // check MOVE
    memset(& arg, 0, sizeof(arg));
    cli.process("MOVE1234\r\n");
    EXPECT_EQ(arg.action, & b);
    EXPECT_EQ(arg.argc, 1);
    EXPECT_EQ(arg.values[0], 1234);

    // check HELP
    memset(& arg, 0, sizeof(arg));
    cli.process("HELP\r\n");
    EXPECT_EQ(arg.action, & c);
    EXPECT_EQ(arg.argc, 0);
}

TEST(Cli, CliSimilar)
{
    CLI cli;
    CliArg arg;

    Action a = { "ABC", on_str, & arg, 0 };
    Action b = { "ABE", on_str, & arg, 0 };
    cli.add_action(& a);
    cli.add_action(& b);

    // check ABC
    memset(& arg, 0, sizeof(arg));
    cli.process("ABC1234,2345\r\n");
    EXPECT_EQ(arg.action, & a);
    EXPECT_EQ(arg.argc, 2);
    EXPECT_EQ(arg.values[0], 1234);
    EXPECT_EQ(arg.values[1], 2345);

    // check ABE
    memset(& arg, 0, sizeof(arg));
    cli.process("ABE1234\r\n");
    EXPECT_EQ(arg.action, & b);
    EXPECT_EQ(arg.argc, 1);
    EXPECT_EQ(arg.values[0], 1234);

    Action c = { "AB", on_str, & arg, 0 };
    cli.add_action(& c);

    // check AB
    memset(& arg, 0, sizeof(arg));
    cli.process("AB1234\r\n");
    EXPECT_EQ(arg.action, & c);
    EXPECT_EQ(arg.argc, 1);
    EXPECT_EQ(arg.values[0], 1234);

    // check ABE no longer matches
    memset(& arg, 0, sizeof(arg));
    cli.process("ABE1234\r\n");
    EXPECT_EQ(arg.action, (void*)0);
    EXPECT_EQ(arg.argc, 0);
}

    /*
     *
     */

typedef struct {
    Action *a;
    int cursor;
    CLI::Error err;
}   ErrorInfo;

static ErrorInfo err_info;

static void err_fn(Action *a, int cursor, CLI::Error err)
{
    err_info.a = a;
    err_info.cursor = cursor;
    err_info.err = err;
}

TEST(Cli, ErrorFn)
{
    CLI cli;
    CliArg arg;

    cli.set_error_fn(err_fn);

    Action a = { "ABC", on_str, & arg, 0 };
    Action b = { "ABE", on_str, & arg, 0 };
    cli.add_action(& a);
    cli.add_action(& b);

    // check ABE matches
    memset(& arg, 0, sizeof(arg));
    cli.process("ABE1234\r\n");
    EXPECT_EQ(arg.action, & b);
    EXPECT_EQ(arg.argc, 1);

    // No matching command
    memset(& arg, 0, sizeof(arg));
    err_fn(0, 0, CLI::ERR_NONE);

    cli.process("X");
    EXPECT_EQ(arg.action, (void*)0);
    EXPECT_EQ(arg.argc, 0);
    EXPECT_EQ(err_info.cursor, 1);
    EXPECT_EQ(err_info.err, CLI::ERR_UNKNOWN_CMD);

    // Fail match part way in
    memset(& arg, 0, sizeof(arg));
    err_fn(0, 0, CLI::ERR_NONE);

    cli.process("ABX");
    EXPECT_EQ(arg.action, (void*)0);
    EXPECT_EQ(arg.argc, 0);
    EXPECT_EQ(err_info.cursor, 3);
    EXPECT_EQ(err_info.err, CLI::ERR_UNKNOWN_CMD);

    // Command match, no sep/num/sign
    memset(& arg, 0, sizeof(arg));
    err_fn(0, 0, CLI::ERR_NONE);

    cli.process("ABEZ");
    EXPECT_EQ(arg.action, (void*)0);
    EXPECT_EQ(arg.argc, 0);
    EXPECT_EQ(err_info.cursor, 4);
    EXPECT_EQ(err_info.err, CLI::ERR_BAD_CHAR);

    // seperators with no number are okay
    memset(& arg, 0, sizeof(arg));
    err_fn(0, 0, CLI::ERR_NONE);

    cli.process("ABE,1,2\r\n");
    EXPECT_EQ(arg.action, & b);
    EXPECT_EQ(arg.argc, 2);
    EXPECT_EQ(arg.values[0], 1);
    EXPECT_EQ(arg.values[1], 2);
    EXPECT_EQ(err_info.cursor, 0);
    EXPECT_EQ(err_info.err, CLI::ERR_NONE);

    // trailing seperators are bad
    memset(& arg, 0, sizeof(arg));
    err_fn(0, 0, CLI::ERR_NONE);

    cli.process("ABE,1,\r\n");
    EXPECT_EQ(arg.action, (void*)0);
    EXPECT_EQ(arg.argc, 0);
    EXPECT_EQ(err_info.cursor, 7);
    EXPECT_EQ(err_info.err, CLI::ERR_NUM_EXPECTED);

    // too many args
    memset(& arg, 0, sizeof(arg));
    err_fn(0, 0, CLI::ERR_NONE);

    cli.process("ABE,1,2,3,4,5,6,7,8,9,0,");
    EXPECT_EQ(arg.action, (void*)0);
    EXPECT_EQ(arg.argc, 0);
    EXPECT_EQ(err_info.cursor, 24);
    EXPECT_EQ(err_info.err, CLI::ERR_TOO_LONG);

    // bad sign
    memset(& arg, 0, sizeof(arg));
    err_fn(0, 0, CLI::ERR_NONE);

    cli.process("ABE,1,3+\r\n");
    EXPECT_EQ(arg.action, (void*)0);
    EXPECT_EQ(arg.argc, 0);
    EXPECT_EQ(err_info.cursor, 8);
    EXPECT_EQ(err_info.err, CLI::ERR_BAD_SIGN);

}

//  FIN
