
#include "gtest/gtest.h"

#include "panglos/debug.h"

static const LUT _x_lut[] = {
    {   "0", 0 },
    {   "1", 1 },
    {   "2", 2 },
    {   "4", 4 },
    {   "8", 8 },
    {   "10", 0x10 },
    {   0   },
};

static void check(const char **expect, int code)
{
    const LUT *iter;
    const LUT *start = _x_lut;
    while (true)
    {
        const char *msg = lut_and(start, code, & iter);
        if (!msg) break;
        EXPECT_STREQ(msg, *expect++);
        start = 0;
    }
    EXPECT_FALSE(*expect);
}

TEST(Lut, LutAnd)
{
    {
        const char *expect[] = { 0 };
        check(expect, 0);
        EXPECT_STREQ("0", lut(_x_lut, 0));
    }
    {
        const char *expect[] = { "1", 0 };
        check(expect, 1);
        EXPECT_STREQ("1", lut(_x_lut, 1));
    }
    {
        const char *expect[] = { "1", "2", 0 };
        check(expect, 3);
        EXPECT_STREQ("unknown", lut(_x_lut, 3));
    }
    {
        const char *expect[] = { "2", "4", 0 };
        check(expect, 6);
        EXPECT_STREQ("unknown", lut(_x_lut, 6));
    }
    {
        const char *expect[] = { "1", "2", "4", 0 };
        check(expect, 7);
        EXPECT_STREQ("unknown", lut(_x_lut, 7));
    }
    {
        const char *expect[] = { "8", 0 };
        check(expect, 8);
        EXPECT_STREQ("8", lut(_x_lut, 8));
    }
    {
        const char *expect[] = { "1", "8", 0 };
        check(expect, 9);
        EXPECT_STREQ("unknown", lut(_x_lut, 9));
    }
}

//  FIN
