
#include <string.h>

#include "panglos/stm32/hal.h"

#include "panglos/debug.h"

#include "panglos/stm32/irq.h"

namespace panglos {

// Need some way to derive this from the HAL/cm4/files ?
#if defined(STM32F446xx)
#define MAX_VECTOR 100 
#endif

// Perhaps don't use the first 16 vectors, so truncate the table a bit?
// NVIC_USER_IRQ_OFFSET is 16 for all MPUs?
// +1 for the stack pointer at the start of the table?
#define IRQ_TO_IDX(irq)     ((irq) + NVIC_USER_IRQ_OFFSET + 1)
#define IPSR_TO_IDX(ipsr)   ((ipsr) & 0x1ff)
#define NUM_VECTORS         MAX_VECTOR

    /*
     *  Array of dispatch handlers
     */

static struct IrqHandler *handlers[NUM_VECTORS];

extern "C" void set_vector(IRQn_Type irq, struct IrqHandler *handler)
{
    const int idx = IRQ_TO_IDX(irq);
    ASSERT(!handlers[idx]);
    if (!handler)
    {
        NVIC_DisableIRQ(irq);
    }
    handlers[idx] = handler;
}

    // Point every (weak) user defined interrupt here.
    // This needs to be added to the irq g_pfnVectors table.

extern "C"
__attribute__((used))
void Dispatch_Handler()
{
    const int idx = IPSR_TO_IDX(__get_IPSR());
    struct IrqHandler *handler = handlers[idx];
    ASSERT(handler);
    handler->run();
}

}   //  namespace panglos


//  FIN
