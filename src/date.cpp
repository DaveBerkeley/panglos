
#include <stdlib.h>

#include "panglos/debug.h"
#include "panglos/date.h"

namespace panglos {

bool DateTime::leap_year(int y)
{
    // we can ignore 2100
    return (y % 4) == 0;
}

int DateTime::days_in_month(int y, int m)
{
    ASSERT((m > 0) && (m <= 12));
    ASSERT((y >= 0) && (y <= 99));
    if (m == 2)
    {
        return leap_year(y) ? 29 : 28;
    }
    static const int days[] = {
        31, // jan
        0,
        31, // mar
        30, // apr
        31, // may
        30, // jun
        31, // jul
        31, // aug
        30, // sep
        31, // oct
        30, // nov
        31, // dec
    };

    return days[m-1];
}

int DateTime::days_in_year(int y)
{
    return leap_year(y) ? 366 : 365;
}

int DateTime::year_day(int y, int m, int d)
{
    int days = 0;

    for (int mm = 1; mm < m; mm++)
    {
        days += days_in_month(y, mm);
    }
    return days + d;
}

static bool parse(const char *text, size_t digits, int *value)
{
    int v = 0;
    for (size_t i = 0; i < digits; i++)
    {
        v *= 10;
        if (!*text) return false;
        v += int(*text++) - '0';
    }
    if (value) *value = v;
    return true;
}

int DateTime::datetime_cmp(const struct DateTime *dt) const
{
    if (yy != dt->yy) return yy - dt->yy;
    if (mm != dt->mm) return mm - dt->mm;
    if (dd != dt->dd) return dd - dt->dd;
    if (h != dt->h) return h - dt->h;
    if (m != dt->m) return m - dt->m;
    if (s != dt->s) return s - dt->s;
    return 0;
}

bool DateTime::validate_dt()
{
    // ignore centuries
    const int y = yy % 100;
    if (y < 0) return false;
    if ((mm < 1) || (mm > 12)) return false;
    if ((dd < 1) || (dd > days_in_month(y, mm))) return false;
    if ((h < 0) || (h > 23)) return false;
    if ((m < 0) || (m > 59)) return false;
    if ((s < 0) || (s > 59)) return false;
    return true;
}

bool DateTime::parse_datetime(const char *text)
{
    if (!parse(text, 4, & yy)) return false;
    if (!parse(& text[4], 2, & mm)) return false;
    if (!parse(& text[6], 2, & dd)) return false;
    if ('T' != text[8]) return false;
    if (!parse(& text[9], 2, & h)) return false;
    if (!parse(& text[11], 2, & m)) return false;
    if (!parse(& text[13], 2, & s)) return false;
    return validate_dt();
}

}   //  namespace panglos

//  FIN
