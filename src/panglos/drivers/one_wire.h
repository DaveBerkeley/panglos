
#pragma once

namespace panglos {

    /*
     *
     */

class OneWire
{
public:
    virtual ~OneWire() { }

    virtual bool reset() = 0;
    virtual bool write(const uint8_t *buff, size_t s) = 0;
    virtual bool read(uint8_t *buff, size_t s) = 0;

    virtual bool find(uint64_t *addr, const uint8_t type) = 0;

    static OneWire *create(int pin);
    static OneWire *create_bitbang(int pin);
};

    /*
     *
     */

class _OneWire : public OneWire
{
public:
    class BusDevice
    {
        BusDevice *next;
    public:
        uint64_t addr;
        bool used;

        BusDevice(uint64_t a)
        :   next(0),
            addr(a),
            used(false)
        {
        }

        static BusDevice **get_next(BusDevice *item)
        {
            return & item->next;
        }
        static int match(BusDevice *item, void*arg)
        {
            return (item->addr & 0xff) == *((uint8_t*) arg);
        } 
    };

    panglos::List<BusDevice*> devices;

    _OneWire()
    :   devices(BusDevice::get_next)
    {
    }

    virtual bool find(uint64_t *addr, uint8_t type) override
    {
        BusDevice *dev = devices.find(BusDevice::match, (void*) & type, 0);
        if (!dev) return false;
        if (addr) *addr = dev->addr;
        return true;
    }
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
bool init_onewire_bitbang(panglos::Device *dev, void *arg);

    /*
     *  TODO : move into temperature sensor related header ...
     */

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
