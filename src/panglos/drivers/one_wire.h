
#pragma once

namespace panglos {

    /*
     *
     */

class TemperatureSensor
{
public:
    virtual ~TemperatureSensor() { }

    virtual bool get_temp(double *t) = 0;
};

    /*
     *
     */

class OneWire
{
public:
    virtual ~OneWire() { }

    virtual TemperatureSensor *get_ds18b20() = 0;

    static OneWire *create(int pin);
};

    /*
     *
     */

struct DefOneWire
{
    int pin;
};

class Device;

bool init_onewire(panglos::Device *dev, void *arg);

}   //  panglos

//  FIN
