
#pragma once

namespace panglos {

struct DateTime
{
    int yy;
    int mm;
    int dd;
    int h;
    int m;
    int s;

    static bool leap_year(int y);
    static int days_in_month(int y, int m);
    static int days_in_year(int y);
    static int year_day(int y, int m, int d);

    bool validate_dt();
    int datetime_cmp(const struct DateTime *dt) const;
    bool parse_datetime(const char *text);
};

}   //  namespace panglos

//  FIN
