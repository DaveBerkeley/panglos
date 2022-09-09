
#include <gtest/gtest.h>

#include "panglos/drivers/nmea.h"

using namespace panglos;

TEST(NMEA, Any)
{
    char line[128];

    const char *gpgga = "$GPGGA,170834,4124.8963,N,08151.6838,W,1,05,1.5,280.2,M,-34.0,M,,,*75\n";
    strncpy(line, gpgga, sizeof(line));

    NMEA::Location loc = { 0 };

    bool ok = NMEA::parse(& loc, line, strlen(line));
    EXPECT_TRUE(ok);

    EXPECT_NEAR(41.414938, loc.lat, 0.0001);
    EXPECT_NEAR(81.861397, loc.lon, 0.0001);
}

//  FIN
