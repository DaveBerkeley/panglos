
#include <gtest/gtest.h>

#include "panglos/drivers/nmea.h"

using namespace panglos;

TEST(NMEA, Checksum)
{
    char line[128];
    const char *nmea = "$GPGSA,A,3,04,05,,09,12,,,24,,,,,2.5,1.3,2.1*39";
    strncpy(line, nmea, sizeof(line));

    bool ok = NMEA::checksum(line);
    EXPECT_TRUE(ok);
}

TEST(NMEA, Any)
{
    char line[128];
    const char *gpgga = "$GPGGA,123519,4807.038,N,01131.000,E,1,08,0.9,545.4,M,46.9,M,,*47\n";
    strncpy(line, gpgga, sizeof(line));

    NMEA::Location loc = { 0 };

    bool ok = NMEA::parse(& loc, line);
    EXPECT_TRUE(ok);

    EXPECT_EQ(12, loc.hms.h);
    EXPECT_EQ(35, loc.hms.m);
    EXPECT_EQ(19, loc.hms.s);
    EXPECT_NEAR(48.117300, loc.lat, 0.001);
    EXPECT_NEAR(11.516667, loc.lon, 0.001);
    EXPECT_NEAR(545.4, loc.alt, 0.1);
}

//  FIN
