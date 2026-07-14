
#pragma once

namespace panglos {

class IrqHandler;
    
extern "C" void set_vector(IRQn_Type irq, IrqHandler *handler);

class IrqHandler
{
public:
    virtual ~IrqHandler() { }
    virtual void run() = 0;
};

}   //  namespace panglos

//  FIN
