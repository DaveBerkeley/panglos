
#include <math.h>

#include <gtest/gtest.h>

#include "panglos/debug.h"

#include "panglos/fmt.h"

using namespace panglos;

TEST(Fmt, Test)
{
    const int width = 4;
    char buff[15];

    struct Item {
        double d;
        const char *match;
        bool okay;
    };

    struct Item items[] = {
        {   1024.0, "1024", true },
        {   9999.9, "9999", true },
        {   -1024.0, "-1k0", true },
        {   -1234.0, "-1k2", true },
        {   100000.0, "100k", true },
        {   100100.0, "100k", true },
        {   10000.0, "10k0", true },
        {   10234.0, "10k2", true },
        {   -10234.0, "-10k", true },
        {   -1234.0, "-1k2", true },
        {   -999.0, "-999", true },
        {   0.012, "12m0", true },
        {   -100e3, "-100k", false }, // Won't fit in 4 chars!
        {   0.000999, "999u", true },
        {   0.0009999, "999u", true },
    };

    for (auto & item : items)
    {
        buff[0] = '\0';
        const bool okay = fmt_eng(buff, sizeof(buff), item.d, width);
        //PO_DEBUG("'%s' %f %e", buff, item.d, item.d);
        EXPECT_EQ(okay, item.okay);
        if (okay)
        {
            EXPECT_STREQ(buff, item.match);
        }
    }
}

//  FIN
