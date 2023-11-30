
#pragma once

namespace panglos {

class ADC 
{
public:
    virtual uint16_t read(uint8_t chan) = 0;
};

}   //  namespace panglos

//  FIN
