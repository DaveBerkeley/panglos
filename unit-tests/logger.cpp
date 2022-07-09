
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

class ThreadPool
{
    Thread **threads;
    int count;

public:
    ThreadPool(const char *name, int n, size_t stack=0, Thread::Priority p=Thread::Medium);
    ~ThreadPool();
    void start(void (*fn)(void *arg), void *arg);
    void join();
};


ThreadPool::ThreadPool(const char *name, int n, size_t stack, Thread::Priority p)
:   threads(0),
    count(n)
{
    threads = new Thread* [n];
    for (int i = 0; i < count; i++)
    {
        threads[i] = Thread::create(name, stack, p);
    }
}

ThreadPool::~ThreadPool()
{
    for (int i = 0; i < count; i++)
    {
        delete threads[i];
    }
    delete[] threads;
}

void ThreadPool::start(void (*fn)(void *), void *arg)
{
    for (int i = 0; i < count; i++)
    {
        threads[i]->start(fn, arg);
    }
}

void ThreadPool::join()
{
    for (int i = 0; i < count; i++)
    {
        threads[i]->join();
    }
}

    /*
     *
     */

struct ThreadInfo {
    Logging *logging;
};

static void thread_test(void *arg)
{
    ASSERT(arg);
    struct ThreadInfo *ti = (struct ThreadInfo*) arg;

    Logging::printf(ti->logging, S_INFO, "hello %s\n", "world");
}

TEST(Logger, Threads)
{
    Logging *logging = new Logging(S_DEBUG);

    StringOut out;

    logging->add(& out, S_INFO);

    ThreadPool pool("x", 10);

    struct ThreadInfo info = {
        .logging = logging,
    };

    pool.start(thread_test, & info);

    pool.join();

    PO_DEBUG("%s", out.get());

    delete logging;
}

//  FIN
