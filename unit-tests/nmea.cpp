
#include <gtest/gtest.h>

#include "panglos/debug.h"
#include "panglos/drivers/nmea.h"

using namespace panglos;

    /*
     *
     */

class NmeaLine
{
public:
    char line[128];

    NmeaLine()
    {
        line[0] = '\0';
    }

    void set(const char *s)
    {
        EXPECT_TRUE(strlen(s) < sizeof(line));
        strncpy(line, s, sizeof(line));
        line[sizeof(line)-1] = '\0';
    }
};

    /*
     *
     */

TEST(NMEA, Strip)
{
    NmeaLine nmea;

    nmea.set("hello\r\n");
    NMEA::strip(nmea.line);
    EXPECT_STREQ("hello", nmea.line);

    nmea.set("hello\r");
    NMEA::strip(nmea.line);
    EXPECT_STREQ("hello", nmea.line);

    nmea.set("hello\n");
    NMEA::strip(nmea.line);
    EXPECT_STREQ("hello", nmea.line);

    nmea.set("hello");
    NMEA::strip(nmea.line);
    EXPECT_STREQ("hello", nmea.line);
}

    /*
     *
     */

TEST(NMEA, Split)
{
    NmeaLine nmea;

    const int num = 20;
    char *parts[num];
    int n;

    nmea.set("one,two,,four");
    n = NMEA::split(nmea.line, parts, num);
    EXPECT_EQ(n, 4);

    nmea.set("one,two,,four,");
    n = NMEA::split(nmea.line, parts, num);
    EXPECT_EQ(n, 5);

    nmea.set(",");
    n = NMEA::split(nmea.line, parts, num);
    EXPECT_EQ(n, 2);

    nmea.set("one");
    n = NMEA::split(nmea.line, parts, num);
    EXPECT_EQ(n, 1);

    nmea.set("");
    n = NMEA::split(nmea.line, parts, num);
    EXPECT_EQ(n, 1);

    nmea.set(",");
    n = NMEA::split(nmea.line, parts, num);
    EXPECT_EQ(n, 2);

    nmea.set(",two,");
    n = NMEA::split(nmea.line, parts, num);
    EXPECT_EQ(n, 3);

    nmea.set(",,,");
    n = NMEA::split(nmea.line, parts, num);
    EXPECT_EQ(n, 4);

    nmea.set(",,,four");
    n = NMEA::split(nmea.line, parts, num);
    EXPECT_EQ(n, 4);
}

    /*
     * Sample streams from :
     *
     * https://campar.in.tum.de/twiki/pub/Chair/NaviGpsDemon/nmea.html#stream
     */

TEST(NMEA, Checksum)
{
    NmeaLine nmea;

    const char *tests[] = {
        "$GPRMC,183729,A,3907.356,N,12102.482,W,000.0,360.0,080301,015.5,E*6F",
        "$GPRMB,A,,,,,,,,,,,,V*71",
        "$GPGGA,183730,3907.356,N,12102.482,W,1,05,1.6,646.4,M,-24.1,M,,*75",
        "$GPGSA,A,3,02,,,07,,09,24,26,,,,,1.6,1.6,1.0*3D",
        "$GPGSV,2,1,08,02,43,088,38,04,42,145,00,05,11,291,00,07,60,043,35*71",
        "$GPGSV,2,2,08,08,02,145,00,09,46,303,47,24,16,178,32,26,18,231,43*77",
        "$PGRME,22.0,M,52.9,M,51.0,M*14",
        "$GPGLL,3907.360,N,12102.481,W,183730,A*33",
        "$PGRMZ,2062,f,3*2D",
        "$PGRMM,WGS 84*06",
        "$GPBOD,,T,,M,,*47",
        "$GPRTE,1,1,c,0*07",
        "$GPRMC,183731,A,3907.482,N,12102.436,W,000.0,360.0,080301,015.5,E*67",
        "$GPRMB,A,,,,,,,,,,,,V*71",
        "$GPGGA,175137.00,5023.37605,N,00408.15089,W,1,05,2.04,94.2,M,50.4,M,,*7F",
        0,
    };

    for (const char **test = tests; *test; test++)
    {
        nmea.set(*test);
        bool ok = NMEA::checksum(nmea.line);
        EXPECT_TRUE(ok);
    }
}

TEST(NMEA, HMS)
{
    char buff[128];
    NMEA::Time hms;

    bool ok;

    strncpy(buff, "123456", sizeof(buff));
    ok = NMEA::parse_hms(& hms, buff);
    EXPECT_TRUE(ok);
    EXPECT_EQ(hms.h, 12);
    EXPECT_EQ(hms.m, 34);
    EXPECT_EQ(hms.s, 56);

    //too few chars
    strncpy(buff, "12345", sizeof(buff));
    ok = NMEA::parse_hms(& hms, buff);
    EXPECT_FALSE(ok);

    // too many chars
    strncpy(buff, "1234567", sizeof(buff));
    ok = NMEA::parse_hms(& hms, buff);
    EXPECT_FALSE(ok);

    // fractions
    strncpy(buff, "112233.000", sizeof(buff));
    ok = NMEA::parse_hms(& hms, buff);
    EXPECT_TRUE(ok);
    EXPECT_EQ(hms.h, 11);
    EXPECT_EQ(hms.m, 22);
    EXPECT_EQ(hms.s, 33);

    // non numbers
    const char *bad[] = {
        "1234a7",
        "123456 123",
        "123456 abc",
        "123456a",
        "-123456",
        "0x1234",
        "a123456",
        0
    };
    for (const char **b = bad; *b; b++)
    {
        strncpy(buff, *b, sizeof(buff));
        ok = NMEA::parse_hms(& hms, buff);
        EXPECT_FALSE(ok);
    }
}

TEST(NMEA, LatLon)
{
    NmeaLine nmea;

    double d;
    bool ok;

    nmea.set("4349.7294");
    ok = NMEA::parse_latlon(& d, nmea.line, "N");
    EXPECT_TRUE(ok);
    EXPECT_NEAR(d, 43.82882, 0.0001);

    nmea.set("0100.0000");
    ok = NMEA::parse_latlon(& d, nmea.line, "N");
    EXPECT_TRUE(ok);
    EXPECT_NEAR(d, 1.0, 0.0001);

    nmea.set("0059.99999");
    ok = NMEA::parse_latlon(& d, nmea.line, "N");
    EXPECT_TRUE(ok);
    EXPECT_NEAR(d, 1.0, 0.0001);

    nmea.set("0000.00000");
    ok = NMEA::parse_latlon(& d, nmea.line, "E");
    EXPECT_TRUE(ok);
    EXPECT_NEAR(d, 0.0, 0.0001);

    nmea.set("17959.99999");
    ok = NMEA::parse_latlon(& d, nmea.line, "E");
    EXPECT_TRUE(ok);
    EXPECT_NEAR(d, 180.0, 0.0001);

    nmea.set("4530.00000");
    ok = NMEA::parse_latlon(& d, nmea.line, "E");
    EXPECT_TRUE(ok);
    EXPECT_NEAR(d, 45.5, 0.0001);

    // West is negative
    nmea.set("00000.294");
    ok = NMEA::parse_latlon(& d, nmea.line, "W");
    EXPECT_TRUE(ok);
    EXPECT_NEAR(d, -0.004900, 0.0001);

    // East is positive
    nmea.set("00000.093");
    ok = NMEA::parse_latlon(& d, nmea.line, "E");
    EXPECT_TRUE(ok);
    EXPECT_NEAR(d, 0.001550, 0.0001);

    // North is positive
    nmea.set("0001.483");
    ok = NMEA::parse_latlon(& d, nmea.line, "N");
    EXPECT_TRUE(ok);
    EXPECT_NEAR(d, 0.024717, 0.0001);

    // South is negative
    nmea.set("0002.966");
    ok = NMEA::parse_latlon(& d, nmea.line, "S");
    EXPECT_TRUE(ok);
    EXPECT_NEAR(d, -0.049433, 0.0001);
}

    /*
     *
     */

TEST(NMEA, GGA)
{
    NmeaLine nmea;
    const char *gga = "$GPGGA,123519,4807.038,N,01131.000,E,1,08,0.9,545.4,M,46.9,M,,*47\n";
    nmea.set(gga);

    NMEA::Location loc = { { 0, }, };

    bool ok = NMEA::parse(& loc, nmea.line);
    EXPECT_TRUE(ok);

    EXPECT_EQ(12, loc.hms.h);
    EXPECT_EQ(35, loc.hms.m);
    EXPECT_EQ(19, loc.hms.s);
    EXPECT_NEAR(48.117300, loc.lat, 0.001);
    EXPECT_NEAR(11.516667, loc.lon, 0.001);
    EXPECT_NEAR(545.4, loc.alt, 0.1);
}

TEST(NMEA, RMC)
{
    NmeaLine nmea;

    {
        const char *rmc = "$GPRMC,081836,A,4807.038,N,01131.000,E,0.413,,110823,,,A*57\n";
        nmea.set(rmc);

        NMEA::Location loc = { { 0, }, };

        bool ok = NMEA::parse(& loc, nmea.line);
        EXPECT_TRUE(ok);

        EXPECT_EQ(8, loc.hms.h);
        EXPECT_EQ(18, loc.hms.m);
        EXPECT_EQ(36, loc.hms.s);
        EXPECT_EQ(2023, loc.ymd.yy);
        EXPECT_EQ(8, loc.ymd.mm);
        EXPECT_EQ(11, loc.ymd.dd);
        EXPECT_NEAR(48.117300, loc.lat, 0.001);
        EXPECT_NEAR(11.516667, loc.lon, 0.001);
    }
    {
        const char *rmc = "$GPRMC,123519,A,4807.038,N,01131.000,E,022.4,084.4,230326,003.1,W*63\n";
        nmea.set(rmc);

        NMEA::Location loc = { { 0, }, };

        bool ok = NMEA::parse(& loc, nmea.line);
        EXPECT_TRUE(ok);

        EXPECT_EQ(12, loc.hms.h);
        EXPECT_EQ(35, loc.hms.m);
        EXPECT_EQ(19, loc.hms.s);
        EXPECT_EQ(2026, loc.ymd.yy);
        EXPECT_EQ(3, loc.ymd.mm);
        EXPECT_EQ(23, loc.ymd.dd);
        EXPECT_NEAR(48.117300, loc.lat, 0.001);
        EXPECT_NEAR(11.516667, loc.lon, 0.001);
    }
}

//  FIN
