
#include <stdint.h>

#include "panglos/debug.h"
#include "panglos/time.h"

namespace panglos {

bool Time::elapsed(tick_t start, tick_t period)
{
    const tick_t now = get();
    const uint32_t elapsed = now - start;
    return elapsed >= period;
}

bool Time::elapsed_update(tick_t *start, tick_t period)
{
    ASSERT(start);
    const tick_t now = get();
    const uint32_t diff = now - *start;
    const bool elapsed = diff >= period;
    if (elapsed)
    {
        *start += period;
    }
    return elapsed;
}

}   //  namespace panglos

//  FIN
