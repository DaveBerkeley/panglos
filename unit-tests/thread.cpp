
#include <atomic>

#include <gtest/gtest.h>

#include "panglos/debug.h"
#include "panglos/time.h"
#include "panglos/thread.h"

using namespace panglos;

static void tt_current(void *arg)
{
    PO_DEBUG("");
    ASSERT(arg);
    //Thread *thread = (Thread*) arg;
    //EXPECT_EQ(thread, Thread::get_current());
}

TEST(Thread, GetCurrent)
{
    Thread *thread = Thread::create(__FUNCTION__, 4000);

    thread->start(tt_current, thread);
    thread->join();

    delete thread;
}

static void tt_nowt(void *arg)
{
    std::atomic<bool> *dead = (std::atomic<bool>*) arg;

    while (true)
    {
        Time::sleep(1);
        if (!dead) break;
        if (*dead) break;
    }
}

TEST(Thread, Pool)
{
    ThreadPool pool("thread_%d", 5, 4000);

    pool.start(tt_nowt, 0);
    pool.join();
}

TEST(Thread, Name)
{
    const int num_threads = 5;
    ThreadPool pool("thread_%d", num_threads, 4000);

    std::atomic<bool> dead(false);
    pool.start(tt_nowt, & dead);

    // wait for them all to start
    Time::sleep(1);

    for (int i = 0; i < num_threads; i++)
    {
        char name[24];
        snprintf(name, sizeof(name), "thread_%d", i);
        Thread *thread = Thread::get_by_name(name);
        EXPECT_TRUE(thread);
        EXPECT_STREQ(name, thread->get_name());
    } 

    Thread *thread = Thread::get_by_name("noname");
    EXPECT_FALSE(thread);

    dead = true;

    pool.join();
}

//  FIN
