
#include <string>

#include <gtest/gtest.h>

#include "panglos/debug.h"
#include "panglos/io.h"
#include "panglos/logger.h"

using namespace panglos;

    /*
     *
     */

class StringOut : public Out
{
public:

    std::string text;

    virtual int tx(const char* data, int n) override
    {
        IGNORE(data);
        text.append(data, n);
        return n;
    }

    void reset()
    {
        text = "";
    }

    const char *get()
    {
        return text.c_str();
    }
};

    /*
     *
     */

static void xprintf(Logging *logging, Severity s, const char *fmt, ...)
    __attribute__((format(printf, 3, 4)));

static void xprintf(Logging *logging, Severity s, const char *fmt, ...)
{
    ASSERT(logging);
    va_list ap;
    va_start(ap, fmt);
    logging->log(s, fmt, ap);
    va_end(ap);
}

    /*
     *
     */

TEST(Logger, Remove)
{
    bool ok;
    Logging *logging = new Logging(S_DEBUG);

    EXPECT_EQ(logging->count(), 0);

    StringOut out;

    logging->add(& out, S_INFO);
    EXPECT_EQ(logging->count(), 1);

    ok = logging->remove(& out);
    EXPECT_TRUE(ok);
    EXPECT_EQ(logging->count(), 0);

    // Remove again (should be an error)
    ok = logging->remove(& out);
    EXPECT_FALSE(ok);
    EXPECT_EQ(logging->count(), 0);

    delete logging;
}

    /*
     *
     */

TEST(Logger, Simple)
{
    Logging *logging = new Logging(S_DEBUG);

    StringOut out;

    logging->add(& out, S_INFO);

    xprintf(logging, S_DEBUG, "debug %d\n", 2345);

    EXPECT_STREQ("", out.get());

    xprintf(logging, S_INFO, "info %d\n", 1234);

    EXPECT_STREQ("info 1234\n", out.get());

    delete logging;
}

//  FIN
