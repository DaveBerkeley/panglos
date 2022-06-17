
#include <stdint.h>

#include "panglos/time.h"

namespace panglos {

bool Time::elapsed(tick_t start, tick_t period)
{
    const tick_t now = get();
    const uint32_t elapsed = now - start;
    return elapsed >= period;
}

}   //  namespace panglos

//  FIN
