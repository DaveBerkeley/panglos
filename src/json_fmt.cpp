
#include <stdio.h>
#include <stdint.h>
#include <time.h>
#include <sys/time.h>

#include "panglos/json_fmt.h"

namespace panglos {

JsonOut::JsonOut(panglos::Out *out)
:   panglos::FmtOut(out),
    objects(0)
{
}

void JsonOut::key(const char *label)
{
    if (objects)
    {
        printf(",");
    }
    objects += 1;
    printf(" \"%s\" : ", label);
}

void JsonOut::key_value(const char *label, const char *text)
{
    key(label);
    printf("\"%s\"", text);
}

void JsonOut::key_value(const char *label, const char *fmt, double d)
{
    key(label);
    printf(fmt, d);
}

void JsonOut::key_value(const char *label, int d)
{
    key(label);
    printf("%d", d);
}

void JsonOut::key_dt(const char *key, const struct tm *tm, const char *zone)
{
    const int year = tm->tm_year + 1900;

    char buff[32];
    snprintf(buff, sizeof(buff), "%04d/%02d/%02dT%02d:%02d:%02d",
            year, uint8_t(tm->tm_mon + 1), tm->tm_mday,
            tm->tm_hour, tm->tm_min, tm->tm_sec);
    key_value(key, buff);

    if (zone)
    {
        key_value("z", zone);
    }
}

void JsonOut::key_dt(const char *key, const char *zone)
{
    struct timeval tv;
    gettimeofday(& tv, NULL);
    struct tm tm;
    gmtime_r(& tv.tv_sec, & tm);
    // test valid year (last 2 digits only)
    const int year = tm.tm_year + 1900;
    if (year < 2023)
    {
        // invalid year
        return;
    }

    key_dt(key, & tm, zone);
}

void JsonOut::start_obj()
{
    printf("%s", "{");
}

void JsonOut::end_obj()
{
    printf("%s", " }");
}

}   //  namespace panglos

//  FIN
