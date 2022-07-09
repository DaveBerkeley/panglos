
#include <gtest/gtest.h>

#include <panglos/debug.h>
#include <panglos/thread.h>
#include <panglos/dispatch.h>

using namespace panglos;

static void irq_task(void *arg)
{
    ASSERT(arg);
    Dispatch *it = (Dispatch*) arg;
    it->run();
}

class Callback : public Dispatch::Callback
{
public:
    const char *text;
    Semaphore *semaphore;

    Callback(const char *text)
    : Dispatch::Callback(text), text(0), semaphore(0)
    {
        semaphore = Semaphore::create();
    }

    ~Callback()
    {
        delete semaphore;
    }

    void execute()
    {
        text = debug;
        ASSERT(semaphore);
        semaphore->post();
    }

    void wait()
    {
        ASSERT(semaphore);
        semaphore->wait();
    }
};

TEST(Dispatch, Test)
{
    Dispatch task;

    Thread *thread = Thread::create("xx");
    thread->start(irq_task, & task);

    Callback cb("hello");

    EXPECT_EQ(0, cb.text);
    task.put(& cb);

    cb.wait();
    EXPECT_STREQ("hello", cb.text);

    task.kill();

    thread->join();
    delete thread;
}

//  FIN
