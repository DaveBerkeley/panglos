
#include "gtest/gtest.h"

#include "panglos/debug.h"
#include "panglos/drivers/rtc.h"

using namespace panglos;

//    static bool parse_time(const char *s, DateTime *dt, const char *fmt=0);
//    static bool validate(const DateTime &dt);

TEST(RTC, Parse)
{
    struct DT
    {
        const char *data;
        const char *fmt;
        bool ok;
        RTC::DateTime result;
    };

    const struct DT dts[] = {
        { "2020/01/01 00:00:00", 0, true, { 0, 0, 0, 1, 1, 2020 } },
        { "2020/11/01 00:00:00", 0, true, { 0, 0, 0, 1, 11, 2020 } },
        { "2020/12/01 00:00:00", 0, true, { 0, 0, 0, 1, 12, 2020 } },
        { "2020/12/01 15:00:00", 0, true, { 0, 0, 15, 1, 12, 2020 } },
        { "2020/12/01 15:45:00", 0, true, { 0, 45, 15, 1, 12, 2020 } },
        { "2020/12/01 15:45:06", 0, true, { 6, 45, 15, 1, 12, 2020 } },
        // fails
        { "2020/13/01 00:00:00", 0, false, },
        { "xxxx/01/01 00:00:00", 0, false, },
        { "2020/01/01T00:00:00", 0, false, },
        { "2020/xx/01 00:00:00", 0, false, },
        { "2020/01/01 24:00:00", 0, false, },
        { "2020/01/01 00:60:00", 0, false, },
        { "2020/01/01 00:00:60", 0, false, },
        // fmts
        { "26-03-15", "%y-%m-%d", true, { 0, 0, 0, 15, 3, 2026 } },
        { "15-Mar-2020", "%d-%b-%Y", true, { 0, 0, 0, 15, 3, 2026 } },
        {   0   },
    };

    for (const struct DT *test = dts; test->data; test++)
    {
        //PO_DEBUG("%s", test->data)
        RTC::DateTime dt;
        bool ok = RTC::parse_time(test->data, & dt, test->fmt);
        EXPECT_EQ(ok, test->ok);
    }
}

//  FIN
