
#include <stdio.h>
#include <math.h>
#include <string.h>

#include "panglos/debug.h"

#include "panglos/fmt.h"

namespace panglos {

    /*
     *  Engineering Formatter : eg "1k56", "330m" "1G8"
     */

bool fmt_eng(char *buff, size_t sz, double d, int width)
{
    //PO_DEBUG("%f %e", d, d);
    snprintf(buff, sz, "%d", int(d));
    const size_t n = strlen(buff);
    // int is the best we can manage
    if (n == size_t(width)) return true;
    if (n == size_t(width-1)) return true;

    const int neg = (d < 0.0) ? 1 : 0;
    const int log = int(log10(fabs(d)));
    const int eng = (log<0) ? (abs(3-log) / 3) : (log / 3);

    const char *hi[] = { "", "k", "M", "G", "T", "P", 0 };
    const char *lo[] = { "", "m", "u", "n", "p", "f", 0 };
    const char *mul = "";

    d = fabs(d);
 
    if (d >= 1000)
    {
        const char **mul = hi;
        while (d >= 1000)
        {
            d /= 1000;
            mul++;
            if (!*mul) return false;
        }        
    }
    if (d < 1)
    {
        const char **mul = lo;
        while (d < 1)
        {
            d *= 1000;
            mul++;
            if (!*mul) return false;
        }        
    }

    if (eng) mul = (log < 0) ? lo[eng] : hi[eng];

    snprintf(buff, sz, "%s%.*f", neg ? "-" : "", 2, d);
    char *s = strchr(buff, '.');
    ASSERT(s);
    // eg -100k won't fit in 4 chars, so fail
    if ((s - buff) >= width) return false;
    *s = *mul;
    buff[width] = '\0';
    //PO_DEBUG("d=%f log=%d lchars=%d eng=%d, mul='%s'", d, log, lchars, eng, mul);
    return true;
}

}   //  namespace panglos

//  FIN
