
    /*
     *  
     */

#pragma once

#include "panglos/drivers/adc.h"

namespace panglos {

class SpiDevice;

class MCP3002 : public ADC
{
    SpiDevice *spi;
public:
    MCP3002(SpiDevice *spi);

    virtual uint16_t read(uint8_t chan) override;
};

}   //  namespace panglos

//  FIN
