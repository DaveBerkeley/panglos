    /*
     *
     */

#include <time.h>

#include "panglos/debug.h"

#include "panglos/date.h"

#include "panglos/drivers/rtc.h"

namespace panglos {

bool RTC::validate(const RTC::DateTime &dt)
{
    if ((dt.yy < 2000) || (dt.yy > 2099)) return false;
    if ((dt.mm < 1)    || (dt.mm > 12))   return false;

    const int days = panglos::DateTime::days_in_month(dt.yy-2000, dt.mm);
    if ((dt.dd < 1)    || (dt.dd >days))  return false;

    if (dt.h > 23) return false;
    if (dt.m > 59)  return false;
    if (dt.s > 59)  return false;
    return true;
}

bool RTC::parse_time(const char *s, RTC::DateTime *dt, const char *fmt)
{
    ASSERT(dt);
    
    if (!fmt)
    {
        fmt = "%Y/%m/%s %H:%M:%S";
    }
    
    struct tm tm = { 0 };
    const char *err = strptime(s, fmt, & tm);
    if (!err)
    {
        PO_DEBUG("Error parsing '%s'", s);
        return false;
    }

    dt->yy = uint16_t(tm.tm_year) + 1970;
    dt->mm = uint8_t(tm.tm_mon) + 1;
    dt->dd = uint8_t(tm.tm_mday);
    dt->h  = uint8_t(tm.tm_hour);
    dt->m  = uint8_t(tm.tm_min);
    dt->s  = uint8_t(tm.tm_sec);

    if (!validate(*dt))
    {
        PO_DEBUG("Error parsing '%s'", s);
        return false;
    }

    return true;
}

}   //  panglos

//  FIN
