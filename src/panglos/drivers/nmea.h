
#include <stdlib.h>

#if !defined(__PANGLOS_NMEA_H__)
#define __PANGLOS_NMEA_H__

namespace panglos {

class NMEA {
    static void strip(char *text, char c);
    static bool parse_latlon(double *f, char *field);
    static bool parse_int(int *d, char *field, int base=10);
    static bool parse_double(double *d, char *field);
public:

    typedef struct {
        int yy;
        int mm;
        int dd;
    }   Date;
    typedef struct {
        int h;
        int m;
        int s;
    }   Time;
    typedef struct {
        Date ymd;
        Time hms;
        double lat;
        double lon;
        double alt;
    }   Location;

    static bool parse(Location *loc, char *data);

    // Utility functions (exposed for unit tests only)
    static void strip(char *text);
    static bool checksum(char *text);
    static int split(char *text, char **parts, int n, char delim=',');
    static bool parse_latlon(double *f, char *field, const char *rose);
    static bool parse_hms(Time *t, char *field);
    static bool parse_dmy(Date *t, char *field);
    static bool gga(NMEA::Location *loc, char **parts, int n);
    static bool rmc(NMEA::Location *loc, char **parts, int n);
};

}   //  namespace panglos

#endif  //  __PANGLOS_NMEA_H__

//  FIN
