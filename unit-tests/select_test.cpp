
#include <pthread.h>

#include <gtest/gtest.h>

#include <panglos/debug.h>
#include <panglos/select.h>

#include "mock.h"

using namespace panglos;

typedef std::map<Semaphore*, bool> Map;

static Map map;

static void *task(void *arg)
{
    ASSERT(arg);
    Select *sel = (Select*) arg;

    Semaphore *sem;

    while (true)
    {
        sem = sel->wait();
        if (!sem)
        {
            break;
        }
        map[sem] = true;
    }

    return 0;
}

TEST(Select, Test)
{
    pthread_t thread;
    int err;

    const int num = 4;
    Semaphore *sems[num];

    Select sel;

    for (int i = 0; i < num; i++)
    {
        Semaphore *s = Semaphore::create();
        sel.add(s);
        sems[i] = s;
        map[s] = false;
    }

    err = pthread_create(& thread, 0, task, & sel);
    EXPECT_EQ(0, err);

    sems[1]->post();
    sems[0]->post();
    sems[3]->post();
    //sems[2]->post();

    // terminate the thread
    sel.post(0);

    err = pthread_join(thread, 0);
    EXPECT_EQ(0, err);

    EXPECT_EQ(true, map[sems[0]]);
    EXPECT_EQ(true, map[sems[1]]);
    EXPECT_EQ(false, map[sems[2]]);
    EXPECT_EQ(true, map[sems[3]]);

    for (int i = 0; i < num; i++)
    {
        delete sems[i];
    }
}

TEST(Select, Timeout)
{
    mock_setup(true);

    Select select;

    // ensure that the event_queue prevents the wait() from blocking
    select.wait(& event_queue, 1000);
    
    mock_teardown();
}

//  FIN
