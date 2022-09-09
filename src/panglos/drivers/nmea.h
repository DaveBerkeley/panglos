
#include <stdlib.h>

#if !defined(__PANGLOS_NMEA_H__)
#define __PANGLOS_NMEA_H__

namespace panglos {

class NMEA {
public:

    typedef struct {
        int h;
        int m;
        int s;
    }   Time;
    typedef struct {
        Time hms;
        double lat;
        double lon;
        double alt;
    }   Location;

    static bool parse(Location *loc, char *data, size_t size);
};

}   //  namespace panglos

#endif  //  __PANGLOS_NMEA_H__

//  FIN
