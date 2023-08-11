
#include <stdlib.h>

#if !defined(__PANGLOS_NMEA_H__)
#define __PANGLOS_NMEA_H__

namespace panglos {

class NMEA {
    bool verbose;

    void strip(char *text, char c);
    bool parse_latlon(double *f, char *field);
    bool parse_int(int *d, char *field, int base=10);
    bool parse_double(double *d, char *field);
public:
    NMEA(bool verbose=false);

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

    bool parse(Location *loc, char *data);

    // Utility functions (exposed for unit tests only)
    void strip(char *text);
    bool checksum(char *text);
    int split(char *text, char **parts, int n, char delim=',');
    bool parse_latlon(double *f, char *field, const char *rose);
    bool parse_hms(Time *t, char *field);
    bool parse_dmy(Date *t, char *field);
    bool gga(NMEA::Location *loc, char **parts, int n);
    bool rmc(NMEA::Location *loc, char **parts, int n);
};

}   //  namespace panglos

#endif  //  __PANGLOS_NMEA_H__

//  FIN
