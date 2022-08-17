
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

    /*
     *
     */

TEST(IO, CharIn)
{
    {
        const char *s = "hello";
        CharIn in(s);

        char buff[128] = { 0 };
        int n = in.rx(buff, sizeof(buff));
        EXPECT_EQ(n, strlen(s));
        EXPECT_STREQ(buff, s);
    }
    {
        const char *s = "hello";
        CharIn in(s);

        char buff[4] = { 0 };
        int n = in.rx(buff, sizeof(buff));
        EXPECT_EQ(n, sizeof(buff));
        EXPECT_FALSE(strncmp(buff, s, n));
    }
    {
        const char *s = "hello";
        CharIn in(s);

        char buff[8] = { 0 };
        int n = in.rx(buff, 4);
        EXPECT_EQ(n, 4);
        EXPECT_FALSE(strncmp(buff, s, n));
        // read the rest
        int m = in.rx(& buff[n], int(sizeof(buff) - n));
        EXPECT_EQ(n + m, strlen(s));
        EXPECT_STREQ(buff, s);
    }
}

    /*
     *
     */

TEST(IO, LineReader)
{
    {
        // line + non-line
        const char *s = "hello\nworld!";
        CharIn in(s);

        LineReader reader(& in);

        char buff[64] = { 0 };
        int n = reader.rx(buff, sizeof(buff));
        EXPECT_EQ(n, 6);
        EXPECT_STREQ(buff, "hello\n");
        int m = reader.rx(& buff[n], int(sizeof(buff) - n));
        EXPECT_EQ(m, 6);
        EXPECT_STREQ(buff, s);
    }
    {
        // line + line
        const char *s = "hello\nworld!\n";
        CharIn in(s);

        LineReader reader(& in);

        char buff[64] = { 0 };
        int n = reader.rx(buff, sizeof(buff));
        EXPECT_EQ(n, 6);
        EXPECT_STREQ(buff, "hello\n");
        n = reader.rx(buff, sizeof(buff));
        EXPECT_EQ(n, 7);
        EXPECT_STREQ(buff, "world!\n");
    }
    {
        // line + empty_line + line
        const char *s = "hello\n\nworld!\n";
        CharIn in(s);

        LineReader reader(& in);

        char buff[64] = { 0 };
        int n = reader.rx(buff, sizeof(buff));
        EXPECT_EQ(n, 6);
        EXPECT_STREQ(buff, "hello\n");

        memset(buff, 0, sizeof(buff));
        n = reader.rx(buff, sizeof(buff));
        EXPECT_EQ(n, 1);
        EXPECT_STREQ(buff, "\n");

        memset(buff, 0, sizeof(buff));
        n = reader.rx(buff, sizeof(buff));
        EXPECT_EQ(n, 7);
        EXPECT_STREQ(buff, "world!\n");
    }
    {
        const char *s = "hello\n\nworld!\n";
        CharIn in(s);

        LineReader reader(& in);

        char buff[64] = { 0 };
        int n = reader.rx(buff, sizeof(buff));
        EXPECT_EQ(n, 6);
        EXPECT_STREQ(buff, "hello\n");
        n = reader.strip(buff, n);
        EXPECT_EQ(n, 5);
        EXPECT_STREQ(buff, "hello");
    }
}

//  FIN
