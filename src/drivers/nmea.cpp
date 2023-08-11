
#include <ctype.h>
#include <string.h>

#include "panglos/debug.h"

#include "panglos/drivers/nmea.h"

namespace panglos {

    /*
     *  Split text into fields, separated by 'delim'
     *
     *  return the number of fields
     */

int NMEA::split(char *text, char **parts, int n, char delim)
{
    for (int i = 0; i < n; i++)
    {
        parts[i] = 0;

        char *s = index(text, delim);
        if (!s)
        {
            // no more delimiters
            return i+1;
        }

        *s = '\0';
        parts[i] = text;
        s += 1;
        if (*s == '\0')
        {
            return i+2;
        }
        text = s;
    }
    return n;
}

    /*
     *
     */

bool NMEA::parse_int(int *d, char *field, int base)
{
    if (!field)
    {
        return false;
    }

    char *end = 0;
    long int val = strtol(field, & end, base);

    if (!end)
    {
        return false;
    }

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
    ASSERT(field);

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

    return true;
}

    /*
     *
     */

bool NMEA::parse_latlon(double *f, char *field, const char *rose)
{
    if (!parse_latlon(f, field))
    {
        return false;
    }

    ASSERT(rose);
    ASSERT(f);

    switch (*rose)
    {
        case 'N' :  break;
        case 'E' :  break;
        case 'S' :  *f = -*f; break;
        case 'W' :  *f = -*f; break;
        default  : return false;
    }

    return true;
}

    /*
     *
     */

bool NMEA::parse_hms(Time *t, char *field)
{
    // time can have fractions of seconds.
    // ignore these.
    char *dot = strchr(field, '.');
    if (dot)
    {
        *dot = '\0';
    }

    if (strlen(field) != 6)
    {
        //PO_ERROR("len %s", field);
        return false;
    }

    int num = 0;
    if (!parse_int(& num, field))
    {
        //PO_ERROR("hhmms %s", field);
        return false;
    }

    t->s = num % 100;
    num /= 100;
    t->m = num % 100;
    num /= 100;
    t->h = num;
    return true;
}

bool NMEA::parse_dmy(Date *d, char *field)
{
    ASSERT(d);
    d->yy = 0;
    d->mm = 0;
    d->dd = 0;

    for (int i = 0; i < 6; i++)
    {
        if (!isdigit(field[i]))
        {
            PO_ERROR("Bad date char at %d : '%c'", i, field[i]);
            return false;
        }
    }

    int idx = 0;
    d->dd  = (field[idx++] - '0') * 10;
    d->dd += (field[idx++] - '0');
    d->mm  = (field[idx++] - '0') * 10;
    d->mm += (field[idx++] - '0');
    d->yy  = 2000 + ((field[idx++] - '0') * 10);
    d->yy += (field[idx++] - '0');

    return field[idx++] == '\0';
}

    /*
     *  http://aprs.gids.nl/nmea/#gga
     */

bool NMEA::gga(NMEA::Location *loc, char **parts, int n)
{
    int idx = 0;
    ASSERT(loc);
    ASSERT(parts[idx++]);
    if (n != 15)
    {
        // Needs all the fields
        PO_ERROR("n=%d", n);
        return false;
    }

    if (!parse_hms(& loc->hms, parts[idx++]))
    {
        PO_ERROR("hhmms '%s'", parts[idx-1]);
        return false;
    }

    if (!parse_latlon(& loc->lat, parts[idx], parts[idx+1]))
    {
        PO_ERROR("lat '%s' '%s'", parts[idx], parts[idx+1]);
        return false;
    }
    idx += 2;

    if (!parse_latlon(& loc->lon, parts[idx], parts[idx+1]))
    {
        PO_ERROR("lon");
        return false;
    }
    idx += 2;

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

    if (!parse_double(& loc->alt, parts[idx++]))
    {
        PO_ERROR("altitude");
        return false;
    }
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

    /*
     *  http://aprs.gids.nl/nmea/#rmc
     */

bool NMEA::rmc(NMEA::Location *loc, char **parts, int n)
{
    int idx = 0;
    ASSERT(loc);
    ASSERT(parts[idx++]);
    if (!((n == 13) || (n == 12)))
    {
        // Needs all enough fields
        PO_ERROR("n=%d", n);
        return false;
    }

    if (!parse_hms(& loc->hms, parts[idx++]))
    {
        PO_ERROR("hhmms '%s'", parts[idx-1]);
        return false;
    }

    if (strcmp("A", parts[idx])) // Active
    {
        if (strcmp("V", parts[idx])) // Void
        {
            PO_ERROR("validity=%s", parts[idx]);
        }
        // Not Valid
        return false;
    }
    idx += 1;

    if (!parse_latlon(& loc->lat, parts[idx], parts[idx+1]))
    {
        PO_ERROR("lat '%s' '%s'", parts[idx], parts[idx+1]);
        return false;
    }
    idx += 2;

    if (!parse_latlon(& loc->lon, parts[idx], parts[idx+1]))
    {
        PO_ERROR("lon");
        return false;
    }
    idx += 2;

    double speed;
    if (!parse_double(& speed, parts[idx++]))
    {
        PO_ERROR("speed");
        return false;
    }

    double course;
    if (!parse_double(& course, parts[idx++]))
    {
        PO_ERROR("course");
        return false;
    }

    if (!parse_dmy(& loc->ymd, parts[idx++]))
    {
        PO_ERROR("YMD");
        return false;
    }

    //  Ignore any trailing fields
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
        return false;
    }

    const int num = 20;
    char *parts[num] = { 0 };

    const int n = split(line, parts, num, ',');
    if (!n)
    {
        PO_ERROR("No Data");
        return false;
    }

    if (!strcmp("$GPGGA", parts[0]))
    {
        return gga(loc, parts, n);
    }

    if (!strcmp("$GPRMC", parts[0]))
    {
        return rmc(loc, parts, n);
    }

    // Not a recognised message
    PO_ERROR("Unknown Message '%s'", parts[0]);
    return false;
}

}   //  namespace panglos

//  FIN
