
#include <gtest/gtest.h>

#include "panglos/debug.h"

#include "panglos/date.h"

using namespace panglos;

TEST(Date, LeapYear)
{
    int ly[] = {
        1996,
        2000, 2004, 2008, 2012, 2016,
        2020, 2024, 2028, 2032, 2036,
    };

    for (int & year : ly)
    {
        EXPECT_TRUE(DateTime::leap_year(year));
    }

    int nly[] = {
        1997, 1998, 1999,
        2001, 2002, 2003,
        2005, 2006, 2007,
        2009, 2010, 2011,
        2013, 2014, 2015,
        2017, 2018, 2019,
        2021, 2022, 2023,
        2025, 2026, 2027,
        2029, 2030, 2031,
        2033, 2034, 2035,
        2037, 2038, 2039,
    };

    for (int & year : nly)
    {
        EXPECT_FALSE(DateTime::leap_year(year));
    }
}

TEST(Date, DaysInMonth)
{
    for (int y = 0; y < 100; y++)
    {
        EXPECT_EQ(DateTime::days_in_month(y, 1), 31); // Jan
        EXPECT_EQ(DateTime::days_in_month(y, 2), DateTime::leap_year(y) ? 29 : 28); // Feb
        EXPECT_EQ(DateTime::days_in_month(y, 3), 31); // Mar
        EXPECT_EQ(DateTime::days_in_month(y, 4), 30); // Apr
        EXPECT_EQ(DateTime::days_in_month(y, 5), 31); // May
        EXPECT_EQ(DateTime::days_in_month(y, 6), 30); // Jun
        EXPECT_EQ(DateTime::days_in_month(y, 7), 31); // Jul
        EXPECT_EQ(DateTime::days_in_month(y, 8), 31); // Aug
        EXPECT_EQ(DateTime::days_in_month(y, 9), 30); // Sep
        EXPECT_EQ(DateTime::days_in_month(y, 10), 31); // Oct
        EXPECT_EQ(DateTime::days_in_month(y, 11), 30); // Nov
        EXPECT_EQ(DateTime::days_in_month(y, 12), 31); // Dec
    }
}

TEST(Date, DaysInYear)
{
    for (int y = 0; y < 100; y++)
    {
        EXPECT_EQ(DateTime::days_in_year(y), DateTime::leap_year(y) ? 366 : 365);
    }
}

#if 0
TEST(Date, YearDay)
{
    for (int y = 0; y < 100; y++)
    {
        // TODO
//int year_day(int y, int m, int d);
    }
}
#endif

TEST(Date, ParseDateTime)
{
    struct Test {
        const char *text;
        struct DateTime dt;
        bool okay;
    };

    struct Test tests[] = {
        {   "20250223T120000", { 2025, 2, 23, 12, 0, 0, }, true },
        {   "19990332T121212", { 0, }, false },
        {   "19990332T1212", { 0, }, false },
        {   "19990332", { 0, }, false },
        {   "19990331T123456", { 1999, 3, 31, 12, 34, 56, }, true },
    };

    for (struct Test & test : tests)
    {
        struct DateTime dt;
        bool okay = dt.parse_datetime(test.text);
        EXPECT_EQ(okay, test.okay);
        if (okay)
        {
            EXPECT_EQ(dt.datetime_cmp(& test.dt), 0);
        }
    }
}

//  FIN
