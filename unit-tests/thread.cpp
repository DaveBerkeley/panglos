
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
    PO_DEBUG("");
    IGNORE(arg);
    Time::sleep(1);
}

TEST(Thread, Pool)
{
    ThreadPool pool("thread_%d", 5, 4000);

    pool.start(tt_nowt, 0);
    pool.join();
}

//  FIN
