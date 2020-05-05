
#include <pthread.h>

#include <gtest/gtest.h>

#include <panglos/debug.h>

#include  <panglos/dispatch.h>

using namespace panglos;

static void *irq_task(void *arg)
{
    ASSERT(arg);
    Dispatch *it = (Dispatch*) arg;
    it->run();
    return 0;
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

    int err;
    pthread_t thread;

    err = pthread_create(& thread, 0, irq_task, & task);
    EXPECT_EQ(0, err);

    Callback cb("hello");

    EXPECT_EQ(0, cb.text);
    task.put(& cb);

    cb.wait();
    EXPECT_STREQ("hello", cb.text);

    task.kill();

    err = pthread_join(thread, 0);
    EXPECT_EQ(0, err);
}

//  FIN
