
#include <pthread.h>

#include <gtest/gtest.h>

#include <panglos/debug.h>
#include <panglos/msg_queue.h>

using namespace panglos;

TEST(MsgQueue, Queue)
{
    std::deque<const char*> q;
    MsgQueue<const char*> mq(& q, 0, 0);

    const char *s;

    s = mq.get();
    EXPECT_EQ(0, s);
    EXPECT_TRUE(mq.empty());

    const char *h = "hello world";
    const char *i = "another";
    const char *j = "string";

    mq.put(h);
    EXPECT_FALSE(mq.empty());

    // should see h
    s = mq.get();
    EXPECT_EQ(h, s);
    EXPECT_TRUE(mq.empty());
    // now empty again
    s = mq.get();
    EXPECT_EQ(0, s);

    mq.put(h);
    mq.put(i);
    mq.put(j);
    EXPECT_FALSE(mq.empty());

    s = mq.get();
    EXPECT_EQ(h, s);
    s = mq.get();
    EXPECT_EQ(i, s);
    s = mq.get();
    EXPECT_EQ(j, s);
    EXPECT_TRUE(mq.empty());

    s = mq.get();
    EXPECT_EQ(0, s);
}

    /*
     *
     */

typedef struct {
    MsgQueue<const char*> *mq;
    Mutex *mutex;
    int do_count;
    int put_count;
    int get_count;
}   ThreadInfo;

static void * put_thread(void *arg)
{
    ASSERT(arg);
    ThreadInfo *ti = (ThreadInfo*) arg;

    while (true)
    {
        {
            Lock lock(ti->mutex);

            if (ti->put_count >= ti->do_count)
            {
                return 0;
            }

            ti->put_count += 1;
        }

        const char *s = "hello";
        ti->mq->put(s);
    }
}

static void * get_thread(void *arg)
{
    ASSERT(arg);
    ThreadInfo *ti = (ThreadInfo*) arg;

    while (true)
    {
        const char *s = ti->mq->wait();
        if (!s)
        {
            return 0;
        }
        EXPECT_STREQ("hello", s);
        ti->get_count += 1;
    }
}

    /*
     *  Test with multiple reader, multiple writer threads
     */

TEST(MsgQueue, ThreadSafe)
{
    std::deque<const char*> q;
    Mutex *mutex = Mutex::create();
    Semaphore *semaphore = Semaphore::create();
    MsgQueue<const char*> mq(& q, mutex, semaphore);

    int put_num = 10, get_num = 10;
    int msgs = 100;
    pthread_t put[put_num], get[get_num];
    int err;

    ThreadInfo ti;
    memset(& ti, 0, sizeof(ti));
    ti.mq = & mq;
    ti.mutex = mutex;
    ti.do_count = msgs;

    ThreadInfo gi[get_num];

    // start lots of get threads
    for (int i = 0; i < get_num; i++)
    {
        memset(& gi[i], 0, sizeof(ThreadInfo));
        gi[i].mq = & mq;
        gi[i].mutex = mutex;
        err = pthread_create(& get[i], 0, get_thread, & gi[i]);
        EXPECT_EQ(0, err);
    }

    // start lots of put threads
    for (int i = 0; i < put_num; i++)
    {
        err = pthread_create(& put[i], 0, put_thread, & ti);
        EXPECT_EQ(0, err);
    }

    // wait for all the put threads to terminate
    for (int i = 0; i < put_num; i++)
    {
        err = pthread_join(put[i], 0);
        EXPECT_EQ(0, err);
    }

    // terminate the list for every listener
    for (int i = 0; i < get_num; i++)
    {
        mq.put(0);
    }

    // wait for all the get threads
    for (int i = 0; i < get_num; i++)
    {
        err = pthread_join(get[i], 0);
        EXPECT_EQ(0, err);
    }

    int sum = 0;
    for (int i = 0; i < get_num; i++)
    {
        sum += gi[i].get_count;
    }

    EXPECT_EQ(msgs, ti.put_count);
    EXPECT_EQ(msgs, sum);

    delete mutex;
    delete semaphore;
}

//  FIN
