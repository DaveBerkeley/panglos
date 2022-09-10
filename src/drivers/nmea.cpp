
#include <string.h>

#include "panglos/debug.h"

#include "panglos/drivers/nmea.h"

namespace panglos {

int NMEA::split(char *text, char **parts, int n, const char *delim)
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
                return i+1;
            }
            continue;
        }

        char *s = strtok_r(text, delim, & save);
        parts[i] = s;
        if (!s)
        {
            return i+1;
        }
        text = 0;
    }
    return n;
}

    /*
     *
     */

bool NMEA::parse_int(int *d, char *field, int base)
{
    ASSERT(field);
    char *end = 0;
    long int val = strtol(field, & end, base);

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

bool NMEA::parse_double(double *d, char *field)
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

bool NMEA::parse_latlon(double *f, char *field)
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

    //PO_DEBUG("%f", *f);

    return true;
}

    /*
     *  http://aprs.gids.nl/nmea/#gga
     */

bool NMEA::gga(NMEA::Location *loc, char **parts, int n)
{
    int idx = 0;
    ASSERT(loc);
    ASSERT(parts[idx++]);
    if (n != 14)
    {
        // Needs all the fields
        PO_ERROR("n=%d", n);
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

    double lat = 0;
    if (!parse_latlon(& lat, parts[idx++]))
    {
        PO_ERROR("lat");
        return false;
    }

    const bool north = parts[idx][0] == 'N';

    if (!strstr("NS", parts[idx++]))
    {
        // Must be N or S
        PO_ERROR("NS");
        return false;
    }
    // Apply N/S correction to lat
    loc->lat = north ? lat : -lat;

    double lon = 0;
    if (!parse_latlon(& lon, parts[idx++]))
    {
        PO_ERROR("lon");
        return false;
    }

    const bool west = parts[idx][0] == 'W';
    if (!strstr("EW", parts[idx++]))
    {
        // Must be E or W
        PO_ERROR("EW");
        return false;
    }
    // Apply E/W correction to lon
    loc->lon = west ? -lon : lon;

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
        PO_ERROR("geoid");
        return false;
    }
    if (!strstr("M", parts[idx++])) // metres
    {
        PO_ERROR("geoid M");
        return false;
    }

    return true;
}

    /*
     *
     */

void NMEA::strip(char *line, char c)
{
    // Remove any nl/cr
    char *nl = strchr(line, c);
    if (nl)
    {
        *nl = '\0';
    }
}

void NMEA::strip(char *line)
{
    strip(line, '\r');
    strip(line, '\n');
}

    /*
     *
     */

bool NMEA::checksum(char *line)
{
    // Checksum test
    char *save = 0;
    char *s = strtok_r(line, "*", & save);
    if (!s)
    {
        PO_ERROR("No '*'");
        return false;
    }
    int cs = 0;
    for (char *t = & line[1]; *t; t++)
    {
        cs ^= int(*t);
    }
    int mcs = 0;
    if (!parse_int(& mcs, save, 16))
    {
        PO_ERROR("parse cs '%s'", save);
        return false;
    }
    if (cs != mcs)
    {
        PO_ERROR("cs=%#x mcs=%#x", cs, mcs);
        return false;
    }

    return true;
}

    /*
     *
     */

bool NMEA::parse(Location *loc, char *line)
{
    ASSERT(line);

    strip(line);
    //PO_DEBUG("%s", line);

    if (!checksum(line))
    {
        PO_ERROR("checksum");
        return false;
    }

    const int num = 20;
    char *parts[num] = { 0 };

    const int n = split(line, parts, num, ",");
    if (!n)
    {
        PO_ERROR("No Data");
        return false;
    }

    if (!strcmp("$GPGGA", parts[0]))
    {
        return gga(loc, parts, n);
    }

    // Not a recognised message
    PO_ERROR("Unknown Message '%s'", parts[0]);
    return false;
}

}   //  namespace panglos

//  FIN
