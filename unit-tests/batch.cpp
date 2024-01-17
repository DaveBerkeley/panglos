
#include "gtest/gtest.h"

#include "panglos/debug.h"
#include "panglos/object.h"
#include "panglos/thread.h"

#include "panglos/batch.h"

using namespace panglos;

TEST(Batch, LifeCycle)
{
    Objects::objects = Objects::create();

    BatchTask* task = BatchTask::start();
    EXPECT_TRUE(task);

    delete task;
    delete Objects::objects;
}

    /*
     *
     */

class BatchTest : public BatchTask::Job
{
public:
    Thread *thread;

    virtual void run() override
    {
        PO_DEBUG("");
        thread = Thread::get_current();
    }

    BatchTest()
    :   thread(0)
    {
    }
};

TEST(Batch, Run)
{
    Objects::objects = Objects::create();

    BatchTask* task = BatchTask::start();
    EXPECT_TRUE(task);

    const int num = 20;
    BatchTest tests[num];

    for (int i = 0; i < num; i++)
    {
        task->execute(& tests[i]);
    }

    // waits for all tasks to complete
    delete task;

    // check they all ran
    Thread *thread = tests[0].thread;
    EXPECT_TRUE(thread);
    for (int i = 0; i < num; i++)
    {
        EXPECT_EQ(tests[i].thread, thread);
    }

    delete Objects::objects;
}

    /*
     *
     */

TEST(Batch, Wait)
{
    Objects::objects = Objects::create();

    BatchTask* task = BatchTask::start();
    EXPECT_TRUE(task);

    {
        BatchTask::WaitJob waiter;
        task->execute(& waiter);
        waiter.wait();
    }

    // waits for all tasks to complete
    delete task;
    delete Objects::objects;
}

//  FIN
