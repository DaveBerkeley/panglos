
#include <gtest/gtest.h>

#include "panglos/debug.h"

#include "panglos/thread.h"
#include "panglos/semaphore.h"
#include "panglos/watchdog.h"

using namespace panglos;

TEST(Watchdog, Basic)
{
    Watchdog::Task *task;
    Time::set(1000);
    Watchdog *w = Watchdog::create(100);
    EXPECT_TRUE(w);

    task = w->check_expired();
    EXPECT_FALSE(task);

    w->poll();
    task = w->check_expired();
    EXPECT_FALSE(task);

    Time::set(1110);
    task = w->check_expired();
    EXPECT_TRUE(task);
    EXPECT_STREQ(task->thread->get_name(), "main");

    w->remove();
    task = w->check_expired();
    EXPECT_FALSE(task);

    w->poll();
    task = w->check_expired();
    EXPECT_FALSE(task);

    w->poll();
    Time::set(1220);
    task = w->check_expired();
    EXPECT_TRUE(task);

    w->remove(task->thread);
    task = w->check_expired();
    EXPECT_FALSE(task);

    delete w;
}

    /*
     *
     */

struct WdTask {
    Watchdog *w;
    Thread *thread;
    Semaphore *sem;
};

static void wd_task(void *arg)
{
    ASSERT(arg);
    struct WdTask *wd = (struct WdTask*) arg;
    wd->sem->post();
    wd->w->poll();
}

TEST(Watchdog, Threads)
{
    Watchdog::Task *task;
    Time::set(1000);
    Watchdog *w = Watchdog::create(100);
    EXPECT_TRUE(w);

    const int num = 5;
    WdTask threads[num];

    for (int i = 0; i < num; i++)
    {
        Thread *thread = Thread::create("");
        struct WdTask *wd = & threads[i];
        wd->w = w;
        wd->thread = thread;
        wd->sem = Semaphore::create();
        thread->start(wd_task, wd);
    }

    // wait for the threads to start
    for (int i = 0; i < num; i++)
    {
        struct WdTask *wd = & threads[i];
        wd->sem->wait();
    }

    task = w->check_expired();
    EXPECT_FALSE(task);

    Time::set(1200);

    task = w->check_expired();
    EXPECT_TRUE(task);

    for (int i = 0; i < num; i++)
    {
        struct WdTask *wd = & threads[i];
        wd->thread->join();
        w->remove(wd->thread);
        delete wd->thread;
        delete wd->sem;
    }

    delete w;
}

//  FIN
