
#include <pthread.h>

#include <gtest/gtest.h>

#include "mock.h"

#include "../debug.h"
#include "../esp8266.h"

using namespace panglos;

class _Output : public Output
{
    virtual int _putc(char c)
    {
        IGNORE(c);
        return 1;
    }
};

static void *runner(void *arg)
{
    ASSERT(arg);
    ESP8266 *radio = (ESP8266*) arg;

    radio->run();

    return 0;
}

TEST(esp8266, Test)
{
    mock_setup(true);

    _Output output;
    Semaphore *s = Semaphore::create();
    RingBuffer rb(128, s);

    ESP8266 radio(& output, & rb, s, 0);

    pthread_t thread;
    int err;

    err = pthread_create(& thread, 0, runner, & radio);
    EXPECT_EQ(0, err);

    sleep(2);
    radio.kill();

    err = pthread_join(thread, 0);
    EXPECT_EQ(0, err);

    delete s;
    mock_teardown();
}

// FIN
