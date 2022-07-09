
#include <string>

#include <gtest/gtest.h>

#include "panglos/debug.h"
#include "panglos/io.h"
#include "panglos/thread.h"
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

TEST(Logger, Severity)
{
    Logging *logging = new Logging(S_DEBUG);

    StringOut out;

    logging->add(& out, S_INFO);

    Logging::printf(logging, S_DEBUG, "debug %d\n", 2345);
    EXPECT_STREQ("", out.get());

    Logging::printf(logging, S_INFO, "info %d\n", 1234);
    EXPECT_STREQ("info 1234\n", out.get());

    delete logging;
}

    /*
     *
     */

struct ThreadInfo {
    Logging *logging;
    const char *s;
};

static void thread_test(void *arg)
{
    ASSERT(arg);
    struct ThreadInfo *ti = (struct ThreadInfo*) arg;

    Logging::printf(ti->logging, S_INFO, "%s", ti->s);
}

TEST(Logger, Threads)
{
    Logging *logging = new Logging(S_DEBUG);

    StringOut out;

    logging->add(& out, S_INFO);

    const int n = 200;
    ThreadPool pool("x", n);

    struct ThreadInfo info = {
        .logging = logging,
        .s = "hello world\n",
    };

    pool.start(thread_test, & info);
    pool.join();

    std::string s;

    for (int i = 0; i < n; i++)
    {
        s += info.s;
    }

    EXPECT_STREQ(s.c_str(), out.get());

    delete logging;
}

//  FIN
