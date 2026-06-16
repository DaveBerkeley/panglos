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
    if ((dt.year < 2000) || (dt.year > 2099)) return false;
    if ((dt.month < 1)   || (dt.month > 12))  return false;

    const int days =  panglos::DateTime::days_in_month(dt.year-2000, dt.month);
    if ((dt.day < 1)     || (dt.day >days))   return false;

    if (dt.hour > 23) return false;
    if (dt.min > 59)  return false;
    if (dt.sec > 59)  return false;
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

    dt->year  = uint16_t(tm.tm_year) + 1970;
    dt->month = uint8_t(tm.tm_mon) + 1;
    dt->day   = uint8_t(tm.tm_mday);
    dt->hour  = uint8_t(tm.tm_hour);
    dt->min   = uint8_t(tm.tm_min);
    dt->sec   = uint8_t(tm.tm_sec);

    if (!validate(*dt))
    {
        PO_DEBUG("Error parsing '%s'", s);
        return false;
    }

    return true;
}

}   //  panglos

//  FIN
