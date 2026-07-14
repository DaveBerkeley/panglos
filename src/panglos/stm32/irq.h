
#pragma once

namespace panglos {

class IrqHandler;
    
extern "C" void set_vector(IRQn_Type irq, IrqHandler *handler);
extern "C" void enable_irq(IRQn_Type irq, uint32_t preempt_level, IrqHandler *handler);

class IrqHandler
{
public:
    virtual ~IrqHandler() { }
    virtual void run() = 0;
};

}   //  namespace panglos

//  FIN
