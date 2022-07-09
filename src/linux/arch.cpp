
#include <stdint.h>

#include "panglos/linux/arch.h"

static bool in_irq = false;

extern "C" bool arch_set_in_irq(bool state)
{
    bool was = in_irq;
    in_irq = state;
    return was;
}

extern "C" bool arch_in_irq()
{
    return in_irq;
}

//  FIN
