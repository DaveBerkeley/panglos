
#include <gtest/gtest.h>

#include <panglos/debug.h>

#include <panglos/time.h>

#include "mock.h"

using namespace panglos;

static void elapsed_test(Time::tick_t ref_time)
{
    time_set(ref_time);

    Time::tick_t start = Time::get();
    EXPECT_EQ(start, ref_time);

    // now == start
    EXPECT_TRUE(Time::elapsed(start, 0));
    EXPECT_FALSE(Time::elapsed(start, 1));
 
    //  now > start
    time_set(ref_time + 100);
    EXPECT_TRUE(Time::elapsed(start, 0));
    EXPECT_TRUE(Time::elapsed(start, 100));
    EXPECT_FALSE(Time::elapsed(start, 101));
    time_set(ref_time + 101);
    EXPECT_TRUE(Time::elapsed(start, 101));
}

TEST(Time, Elapsed)
{
    TickState ts(false);

    elapsed_test(0);
    elapsed_test(1000);
    // Check near rollover
    elapsed_test(0xffffffff - 200);
    elapsed_test(0xfffffff0);
}

//  FIN
