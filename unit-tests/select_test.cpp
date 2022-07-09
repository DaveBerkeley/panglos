
#include <map>

#include <gtest/gtest.h>

#include <panglos/debug.h>
#include <panglos/thread.h>
#include <panglos/select.h>

#include "mock.h"

using namespace panglos;

typedef std::map<Semaphore*, bool> Map;

static Map map;

static Semaphore *dead = 0;

static void task(void *arg)
{
    ASSERT(arg);
    Select *sel = (Select*) arg;

    Semaphore *sem;

    while (true)
    {
        sem = sel->wait();
        if (sem == dead)
        {
            break;
        }
        map[sem] = true;
    }
}

TEST(Select, Test)
{
    Thread *thread;

    const int num = 4;
    Semaphore *sems[num];
    dead = Semaphore::create();

    {
        Select sel(10);

        for (int i = 0; i < num; i++)
        {
            Semaphore *s = Semaphore::create();
            sel.add(s);
            sems[i] = s;
            map[s] = false;
        }

        sel.add(dead);

        thread = Thread::create("xx");
        thread->start(task, & sel);

        sems[1]->post();
        sems[0]->post();
        sems[3]->post();
        //sems[2]->post();
        
        // terminate
        dead->post();

        thread->join();
        delete thread;

        EXPECT_EQ(true, map[sems[0]]);
        EXPECT_EQ(true, map[sems[1]]);
        EXPECT_EQ(false, map[sems[2]]);
        EXPECT_EQ(true, map[sems[3]]);
    }

    for (int i = 0; i < num; i++)
    {
        delete sems[i];
    }

    delete dead;
    dead = 0;
}

TEST(Select, Timeout)
{
    mock_setup(true);

    Select select(10);

    // ensure that the event_queue prevents the wait() from blocking
    select.wait(& event_queue, 1000);
    
    mock_teardown();
}

    /*
     *
     */

TEST(Select, Remove)
{
    Select select(100);

    Semaphore *s = Semaphore::create();

    select.add(s);
    select.remove(s);

    s->post();

    delete s;
}

    /*
     *
     */

class TestSem : public Semaphore
{
public:
    PostHook *hook;

    TestSem() : hook(0) { }

    virtual void post()
    {
        if (hook)
        {
            hook->post(this);
        }
    }
    virtual void wait()
    {
    }
    void set_hook(PostHook *_hook)
    {
        hook = _hook;
    }
};

TEST(Select, Dtor)
{
    TestSem sema, semb, semc;

    EXPECT_EQ(0, sema.hook);
    EXPECT_EQ(0, semb.hook);
    EXPECT_EQ(0, semc.hook);

    {
        Select select(100);

        select.add(& sema);
        select.add(& semb);
        select.add(& semc);

        // check the semaphores are unhooked
        EXPECT_EQ(& select, sema.hook);
        EXPECT_EQ(& select, semb.hook);
        EXPECT_EQ(& select, semc.hook);    
    }

    // check the semaphores are unhooked
    EXPECT_EQ(0, sema.hook);
    EXPECT_EQ(0, semb.hook);
    EXPECT_EQ(0, semc.hook);    
}

TEST(Select, Delete)
{
    TestSem sema, semb, semc;

    Select *select = new Select(100);

    select->add(& sema);
    select->add(& semb);
    select->add(& semc);

    // check the semaphores are unhooked
    EXPECT_EQ(select, sema.hook);
    EXPECT_EQ(select, semb.hook);
    EXPECT_EQ(select, semc.hook);    

    delete select;

    // check the semaphores are unhooked
    EXPECT_EQ(0, sema.hook);
    EXPECT_EQ(0, semb.hook);
    EXPECT_EQ(0, semc.hook);    
}

//  FIN
