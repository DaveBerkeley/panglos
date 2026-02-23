
#include <gtest/gtest.h>

#include "panglos/json_fmt.h"

using namespace panglos;

TEST(JsonFmt, Test)
{
    char buff[128];
    CharOut line(buff, sizeof(buff));
    JsonOut json(& line);

    json.start_obj();
    json.key_value("co2", 1234);
    json.key_value("dev", "hello");

    struct tm tm = { 
        .tm_sec = 50,
        .tm_min = 23,
        .tm_hour = 15,
        .tm_mday = 10,
        .tm_mon = 3, // Apr
        .tm_year=2025-1900,
    };

    json.key_dt("time", & tm, "utc");
    json.key_value("f", "%.4f", 1.2345);
    json.end_obj();

    EXPECT_STREQ(buff,
        "{ \"co2\" : 1234,"
        " \"dev\" : \"hello\","
        " \"time\" : \"2025/04/10T15:23:50\","
        " \"z\" : \"utc\","
        " \"f\" : 1.2345"
        " }"
    );
}

//  FIN
