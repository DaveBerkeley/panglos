
#include <string.h>

#include "panglos/debug.h"
#include "panglos/list.h"
#include "panglos/time.h"
#include "panglos/object.h"
#include "panglos/device.h"

#include "panglos/drivers/one_wire.h"
#include "panglos/drivers/temperature.h"

#include "ds18b20.h"

namespace panglos {

    /*
     *
     */

class DS18B20_Sensor : public TemperatureSensor
{
public:
    enum Resolution
    {
        R_9_BIT = 0,
        R_10_BIT,
        R_11_BIT,
        R_12_BIT,
    };

    static uint8_t device_code()
    {
        return 0x28;
    }

private:

    OneWire *onewire;
    uint64_t addr;
    Resolution resolution;
    Time::tick_t req_time;
    double temperature;

    bool command(const uint8_t *buff, size_t s)
    {
        if (!onewire->reset())
        {
            PO_ERROR("onewwire->reset()");
            return false;
        }

        const size_t sz = sizeof(addr);
        uint8_t tx[1 + sz];

        tx[0] = 0x55; // ROM Command
        memcpy(& tx[1], & addr, sz);

        if (!onewire->write(tx, sizeof(tx)))
        {
            PO_ERROR("write addr error");
            return false;
        }

        return onewire->write(buff, s);
    }

    virtual bool start_conversion() override
    {
        req_time = Time::get();
        uint8_t tx[1] =  { 0x44 }; // Convert Request
        return command(tx, sizeof(tx));
    }

    virtual bool ready()
    {
        Time::tick_t dt = 0;

        switch (resolution)
        {
            case R_9_BIT :  dt = 10 ; break;
            case R_10_BIT : dt = 20 ; break; 
            case R_11_BIT : dt = 40 ; break;
            case R_12_BIT : dt = 80 ; break;
            default : ASSERT(0);
        }

        return Time::elapsed(req_time, dt);
    }

    bool read(double *temp)
    {
        if (!onewire->reset())
        {
            PO_ERROR("onewwire->reset()");
            return false;
        }

        uint8_t tx[1] =  { 0xbe }; //  Read Command
        command(tx, sizeof(tx));

        uint8_t rx[9];
        onewire->read(rx, sizeof(rx));

        const uint16_t ut = rx[0] + (rx[1] << 8);
        const int16_t t = ut;
        double mul = 0;

        switch (resolution)
        {
            case R_9_BIT :  mul = 0.5 ; break;
            case R_10_BIT : mul = 0.25 ; break; 
            case R_11_BIT : mul = 0.125 ; break;
            case R_12_BIT : mul = 0.0625 ; break;
        }
        
        if (temp) *temp = mul * t;
        return true;
    }

    virtual bool get_temp(double *t) override
    {
        if (!ready())
        {
            return false;
        }

        if (!read(& temperature))
        {
            PO_ERROR("error reading temperature");
            return false;
        }

        if (t) *t = temperature;
        return true;
    }

public:
    DS18B20_Sensor(OneWire *o, uint64_t address, Resolution resolution=R_12_BIT)
    :   onewire(o),
        addr(address),
        resolution(R_12_BIT),
        req_time(0),
        temperature(85.0)
    {
    }

    bool set_resolution(Resolution r)
    {
        resolution = r;
        uint8_t code = 0;

        switch (resolution)
        {
            case R_9_BIT :  code = 0x1F ; break;
            case R_10_BIT : code = 0x3F ; break; 
            case R_11_BIT : code = 0x5F ; break;
            case R_12_BIT : code = 0x7F ; break;
        }

        // Set the resolution
        uint8_t tx[4];
        tx[0] = 0x4e; // Write Scratchpad
        tx[1] = 0;
        tx[2] = 0;
        tx[3] = code; // sets the reolution
        return command(tx, sizeof(tx));
    }

    bool init()
    {
        if (!onewire->reset())
        {
            PO_ERROR("onewwire->reset()");
            return false;
        }

        if (!set_resolution(resolution))
        {
            PO_ERROR("set_resolution()");
            return false;
        }

        return start_conversion();
    }
};

bool init_ds18b20(Device *dev, void *arg)
{
    ASSERT(arg);
    const char *name = (const char *) arg;
    OneWire *onewire = (OneWire*) Objects::objects->get(name);
    if (!onewire)
    {
        PO_ERROR("No OneWire device '%s' found", name);
        return false;
    }

    uint64_t addr;
    if (!onewire->find(& addr, DS18B20_Sensor::device_code()))
    {
        PO_ERROR("No device found");
        return false;
    }

    DS18B20_Sensor *temp = new DS18B20_Sensor(onewire, addr);

    if (!temp->init())
    {
        PO_ERROR("Error in DS18B20_Sensor::init()");
        delete temp;
        return false;
    }
 
    Objects::objects->add(dev->name, temp);

    return temp;
}

}   //  panglos

//  FIN
