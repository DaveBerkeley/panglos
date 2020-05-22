
#include <stdlib.h>
#include <stdarg.h>

#include <gtest/gtest.h>

#include <panglos/sprintf.h>

using namespace panglos;

    /*
     *  Test reading of "%" data
     */

static void _test(Format *f, const char** fmt, va_list va)
{
    f->get(fmt, va);
}

static void test(Format *f, const char** fmt, ...)
{
    va_list va;

    va_start(va, fmt);
    _test(f, fmt, va);
    va_end(va);
}

TEST(Printf, Fmt)
{
    {
        Format format;

        const char* fmt = "%s";
        test(& format, & fmt);
        EXPECT_EQ('\0', *fmt);
        EXPECT_EQ('\0', format.flags);
        EXPECT_EQ(0, format.width);
        EXPECT_EQ(0, format.precision);
        EXPECT_STREQ("", format.length);
        EXPECT_EQ('s', format.specifier);
    }
    {
        Format format;

        const char* fmt = "%-3d";
        test(& format, & fmt);
        EXPECT_EQ('\0', *fmt);
        EXPECT_EQ('-', format.flags);
        EXPECT_EQ(3, format.width);
        EXPECT_EQ(0, format.precision);
        EXPECT_STREQ("", format.length);
        EXPECT_EQ('d', format.specifier);
    }
    {
        Format format;

        const char* fmt = "%+15.6f";
        test(& format, & fmt);
        EXPECT_EQ('\0', *fmt);
        EXPECT_EQ('+', format.flags);
        EXPECT_EQ(15, format.width);
        EXPECT_EQ(6, format.precision);
        EXPECT_STREQ("", format.length);
        EXPECT_EQ('f', format.specifier);
    }
    {
        Format format;

        const char* fmt = "%+15.6f";
        test(& format, & fmt);
        EXPECT_EQ('\0', *fmt);
        EXPECT_EQ('+', format.flags);
        EXPECT_EQ(15, format.width);
        EXPECT_EQ(6, format.precision);
        EXPECT_STREQ("", format.length);
        EXPECT_EQ('f', format.specifier);
    }
    {
        Format format;

        const char* fmt = "%lld";
        test(& format, & fmt);
        EXPECT_EQ('\0', *fmt);
        EXPECT_EQ('\0', format.flags);
        EXPECT_EQ(0, format.width);
        EXPECT_EQ(0, format.precision);
        EXPECT_STREQ("ll", format.length);
        EXPECT_EQ('d', format.specifier);
    }
    {
        Format format;

        const char* fmt = "%*d";
        test(& format, & fmt, 20);
        EXPECT_EQ('\0', *fmt);
        EXPECT_EQ('\0', format.flags);
        EXPECT_EQ(20, format.width);
        EXPECT_EQ(0, format.precision);
        EXPECT_STREQ("", format.length);
        EXPECT_EQ('d', format.specifier);
    }
}

    /*
     *
     */

class TestOutput : public Output
{
public:
    char buff[1024];
    size_t idx;

    TestOutput()
    : idx(0)
    {
        buff[0] = '\0';
    }

    void reset()
    {
        idx = 0;
    }

    virtual int _putc(char c)
    {
        EXPECT_TRUE(idx < sizeof(buff));
        buff[idx++] = c;
        if (idx < sizeof(buff))
        {
            buff[idx] = '\0';
        }
        return 1;
    }
};

    /*
     *
     */

TEST(Printf, Num)
{
    // int _print_num(Output *output, const Format *format, int number)

    {
        Format format;
        const char *fmt = "%d";
        test(& format, & fmt);

        TestOutput out;
        _print_num(& out, & format, 1234, 10);
        EXPECT_STREQ("1234", out.buff);
    }
    {
        Format format;
        const char *fmt = "%+d";
        test(& format, & fmt);

        TestOutput out;
        _print_num(& out, & format, 1234, 10);
        EXPECT_STREQ("+1234", out.buff);
    }
    {
        Format format;
        const char *fmt = "%+d";
        test(& format, & fmt);

        TestOutput out;
        _print_num(& out, & format, -1234, 10);
        EXPECT_STREQ("-1234", out.buff);
    }
    {
        Format format;
        const char *fmt = "%08d";
        test(& format, & fmt);

        TestOutput out;
        _print_num(& out, & format, 1234, 10);
        EXPECT_STREQ("00001234", out.buff);
    }
    {
        Format format;
        const char *fmt = "%+08d";
        test(& format, & fmt);

        TestOutput out;
        _print_num(& out, & format, 1234, 10);
        EXPECT_STREQ("+0001234", out.buff);
    }
    {
        Format format;
        const char *fmt = "%08x";
        test(& format, & fmt);

        TestOutput out;
        _print_num(& out, & format, 0xdeadface, 16);
        EXPECT_STREQ("deadface", out.buff);
    }
}

    /*
     *
     */

TEST(Printf, Test)
{
    // strings
    {
        // plain text : no formatting
        TestOutput out;
        out.printf("hello world\r\n");
        EXPECT_STREQ("hello world\r\n", out.buff);
    }
    {
        TestOutput out;
        out.printf("hello %s end", "dave");
        EXPECT_STREQ("hello dave end", out.buff);
    }
    {
        TestOutput out;
        out.printf("'%10s'", "string");
        EXPECT_STREQ("'    string'", out.buff);
    }
    {
        TestOutput out;
        out.printf("'%4s'", "string");
        EXPECT_STREQ("'string'", out.buff);
    }
    {
        TestOutput out;
        out.printf("'%-10s'", "string");
        EXPECT_STREQ("'string    '", out.buff);
    }

    // char
    {
        TestOutput out;
        out.printf("hello %c end", 'X');
        EXPECT_STREQ("hello X end", out.buff);
    }

    // integers
    {
        TestOutput out;
        out.printf("hello %d end", 1234);
        EXPECT_STREQ("hello 1234 end", out.buff);
    }

    // hex
    {
        TestOutput out;
        out.printf("hello %x end", 0x1234);
        EXPECT_STREQ("hello 1234 end", out.buff);
    }

    // hex
    {
        TestOutput out;
        out.printf("hello %#x end", 0x1234);
        EXPECT_STREQ("hello 0x1234 end", out.buff);
    }

    // pointers    
    {
        TestOutput out;
        out.printf("hello %p end", 0x80001234);
        EXPECT_STREQ("hello 0x80001234 end", out.buff);
    }
}

    /*
     *
     */

TEST(Buffered, Test)
{
    TestOutput to;
    Output *out = Output::create_buffered(& to, 32);

    out->_puts("hello", 5);
    EXPECT_EQ(0, to.idx);
    out->_puts("\r", 1);
    EXPECT_EQ(0, to.idx);
    out->_puts("\n", 1);
    EXPECT_EQ(7, to.idx);
    EXPECT_STREQ("hello\r\n", to.buff);

    to.reset();
    EXPECT_EQ(0, to.idx);

    out->_puts("hello\r\nworld!", 13);
    EXPECT_EQ(7, to.idx);
    EXPECT_STREQ("hello\r\n", to.buff);

    to.reset();
    EXPECT_EQ(0, to.idx);
 
    out->_puts("xx\r\n", 4);
    EXPECT_EQ(10, to.idx);
    EXPECT_STREQ("world!xx\r\n", to.buff);

    to.reset();
    EXPECT_EQ(0, to.idx);
 
    out->_puts("abcd", 4);
    EXPECT_EQ(0, to.idx);
    out->flush();
    EXPECT_EQ(4, to.idx);
    EXPECT_STREQ("abcd", to.buff);

    delete out;
}

//  FIN
