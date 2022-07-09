
#include <gtest/gtest.h>

#include "panglos/debug.h"

#include "panglos/io.h"

using namespace panglos;

TEST(IO, CharOut)
{
    char buff[128] = { 0 };
    CharOut co(buff, sizeof(buff));

    FmtOut fmt(& co);

    for (int i = 0; i < 200; i++)
    {
        fmt.printf("%c", i);
    }

    for (size_t i = 0; i < sizeof(buff); i++)
    {
        EXPECT_EQ(i, buff[i]);
    }
}

TEST(IO, LineOut)
{
    char buff[128] = { 0 };
    CharOut co(buff, sizeof(buff));

    {
        // remove "\r\n"
        LineOut lo(200, & co, true);
        FmtOut fmt(& lo);

        fmt.printf("hello ");
        EXPECT_STREQ(buff, "");
        fmt.printf("world!\r\nmore");
        EXPECT_STREQ(buff, "hello world!");
        co.reset();

        fmt.printf(" text\nthe end");
        EXPECT_STREQ(buff, "more text");
        co.reset();

        fmt.printf("\r\n");
        EXPECT_STREQ(buff, "the end");
        co.reset();
    }
    {
        // keep "\r\n"
        LineOut lo(200, & co, false);
        FmtOut fmt(& lo);

        fmt.printf("hello ");
        EXPECT_STREQ(buff, "");
        fmt.printf("world!\r\nmore");
        EXPECT_STREQ(buff, "hello world!\r\n");
        co.reset();

        fmt.printf(" text\nthe end");
        EXPECT_STREQ(buff, "more text\n");
        co.reset();

        fmt.printf("\r\n");
        EXPECT_STREQ(buff, "the end\r\n");
        co.reset();
    }
    {
        // short
        LineOut lo(4, & co, false);
        FmtOut fmt(& lo);

        fmt.printf("hello ");
        EXPECT_STREQ(buff, "hell");
        co.reset();

        fmt.printf("w");
        EXPECT_STREQ(buff, "");

        fmt.printf("o");
        EXPECT_STREQ(buff, "");

        fmt.printf("r");
        EXPECT_STREQ(buff, "o wo");
        co.reset();

        fmt.printf("ld");
        EXPECT_STREQ(buff, "");

        lo.tx_flush();
        EXPECT_STREQ(buff, "rld");
    }
}

//  FIN
