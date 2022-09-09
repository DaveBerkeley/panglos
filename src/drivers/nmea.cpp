
#include <string.h>

#include "panglos/debug.h"

#include "panglos/drivers/nmea.h"

namespace panglos {

static int split(char *text, char **parts, int n, const char *delim=",")
{
    char *save = 0;

    for (int i = 0; i < n; i++)
    {
        parts[i] = 0;

        // if the field is empty, save will point to ","
        // but strtok_r() will not produce an empty field
        if (save && ((*delim) == (*save)))
        {
            // register empty fields
            parts[i] = (char*) "";
            // skip the demiliter
            save += 1;
            if (!*save)
            {
                // EOL
                break;
            }
            continue;
        }

        char *s = strtok_r(text, delim, & save);
        if (!s)
        {
            return i;
        }
        parts[i] = s;
        text = 0;
    }
    return n;
}

    /*
     *
     */

static bool parse_int(int *d, char *field)
{
    ASSERT(field);
    char *end = 0;
    long int val = strtol(field, & end, 10);

    ASSERT(end);
    if (*end)
    {
        // more data on the line
        return false;
    }

    if (d)
    {
        *d = (int) val;
    }

    return true;
}

static bool parse_double(double *d, char *field)
{
    //PO_DEBUG("%s", field);
    ASSERT(field);
    char *end = 0;
    float val = strtof(field, & end);

    ASSERT(end);
    if (*end)
    {
        // more data on the line
        PO_ERROR("%s", end);
        return false;
    }

    if (d)
    {
        *d = val;
    }

    return true;
}

    /*
     *
     */

static bool parse_latlon(double *f, char *field)
{
    // The NMEA lat/lon format is bonkers

    // find 2 chars in front of the '.'
    // this is the start of the 'minutes' field
    char *div = strchr(field, '.');
    if (!div)
    {
        // No decimal point
        return false;
    }

    div -= 2;
    if (div <= field)
    {
        // decimal point appears too early
        return false;
    }

    // convert the minutes
    double minutes = 0;
    if (!parse_double(& minutes, div))
    {
        return false;
    }

    // now '\0' terminate the degrees string
    *div = '\0';
    // read the degrees as an int
    int degrees = 0;
    if (!parse_int(& degrees, field))
    {
        return false;
    }

    // the angle is degrees + (minutes / 60.0)
    ASSERT(f);
    *f = (minutes / 60.0) + degrees;

    PO_DEBUG("%f", *f);

    return true;
}

    /*
     *  http://aprs.gids.nl/nmea/#gga
     */

static bool gga(NMEA::Location *loc, char **parts, int n)
{
    int idx = 0;
    PO_DEBUG("n=%d", n);
    ASSERT(loc);
    ASSERT(parts[idx++]);
    if (n != 16)
    {
        // Needs all the fields
        return false;
    }

    int num = 0;
    if (!parse_int(& num, parts[idx++]))
    {
        PO_ERROR("hhmms");
        return false;
    }

    loc->hms.s = num % 100;
    num /= 100;
    loc->hms.m = num % 100;
    num /= 100;
    loc->hms.h = num;
    PO_DEBUG("%02d:%02d:%02d", loc->hms.h, loc->hms.m, loc->hms.s);

    double lat = 0;
    if (!parse_latlon(& lat, parts[idx++]))
    {
        PO_ERROR("lat");
        return false;
    }
    if (!strstr("NS", parts[idx++]))
    {
        // Must be N or S
        PO_ERROR("NS");
        return false;
    }
    // TODO
    // Apply N/S correction to lat
    loc->lat = lat;

    double lon = 0;
    if (!parse_latlon(& lon, parts[idx++]))
    {
        PO_ERROR("lon");
        return false;
    }
    if (!strstr("EW", parts[idx++]))
    {
        // Must be E or W
        PO_ERROR("EW");
        return false;
    }
    // TODO
    // Apply E/W correction to lon
    loc->lon = lon;

    if (!strstr("012", parts[idx++]))
    {
        // Fix Quality
        PO_ERROR("Fix");
        return false;
    }

    int satellites = 0;
    if (!parse_int(& satellites, parts[idx++]))
    {
        PO_ERROR("satellites");
        return false;
    }

    // Horizontal Dilution of Precision
    // ignore it parts[8]
    idx++;

    double altitude = 0;
    if (!parse_double(& altitude, parts[idx++]))
    {
        PO_ERROR("altitude");
        return false;
    }
    loc->alt = altitude;
    if (!strstr("M", parts[idx++])) // metres
    {
        PO_ERROR("alt M");
        return false;
    }
    double geoid = 0;
    if (!parse_double(& geoid, parts[idx++]))
    {
        return false;
    }
    if (!strstr("M", parts[idx++])) // metres
    {
        PO_ERROR("geoid M");
        return false;
    }

    return true;
}

bool NMEA::parse(Location *loc, char *line, size_t size)
{
    IGNORE(loc);
    PO_DEBUG("%s", line);
    IGNORE(size);

    // TODO : checksum test

    const int num = 20;
    char *parts[num];

    const int n = split(line, parts, num, ",");
    if (!n)
    {
        PO_ERROR("No Data");
        return false;
    }

    for (int i = 0; i < n; i++)
    {
        PO_DEBUG("%d %s", i, parts[i]);
    }

    if (!strcmp("$GPGGA", parts[0]))
    {
        return gga(loc, parts, n);
    }

    // Not a recognised message
    return false;
}

}   //  namespace panglos

//  FIN
