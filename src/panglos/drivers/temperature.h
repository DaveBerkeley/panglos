
#pragma once

namespace panglos {

class TemperatureSensor
{
public:
    virtual ~TemperatureSensor() { }

    virtual bool get_temp(double *t) = 0;
    virtual bool start_conversion() = 0;
    virtual bool ready() = 0;
};

}   //  panglos

//  FIN
