
#if defined(ONEWIRE_RMT)

#include <string.h>

#include "onewire_bus.h"
#include "onewire_device.h"

#include "panglos/debug.h"
#include "panglos/time.h"
#include "panglos/list.h"
#include "panglos/device.h"
#include "panglos/object.h"

#include "panglos/drivers/one_wire.h"

namespace panglos {

class OneWireRmt : public _OneWire
{
    onewire_bus_handle_t bus;

    virtual bool reset() override
    {
        esp_err_t err = onewire_bus_reset(bus);
        return err == ESP_OK;
    }

    virtual bool write(const uint8_t *buff, size_t s) override
    {
        esp_err_t err = onewire_bus_write_bytes(bus, buff, s);
        return err == ESP_OK;
    }

    virtual bool read(uint8_t *buff, size_t s) override
    {
        esp_err_t err = onewire_bus_read_bytes(bus, buff, s);
        return err == ESP_OK;
    }

public:
    OneWireRmt()
    {
    }

    ~OneWireRmt()
    {
        esp_err_t err = onewire_bus_del(bus);
        if (err != ESP_OK)
        {
            PO_ERROR("onewire_bus_rmt_del()");
        }
    }

    bool init(uint32_t pin, uint32_t max_bytes=10)
    {
        PO_DEBUG("");

        onewire_bus_config_t bus_config = {
            .bus_gpio_num = (int) pin,
            .flags = {
                .en_pull_up = true, // enable the internal pull-up resistor in case the external device didn't have one
            }
        };
        onewire_bus_rmt_config_t rmt_config = {
            .max_rx_bytes = max_bytes, // 1byte ROM command + 8byte ROM number + 1byte device command
        };

        // create the bus
        esp_err_t err = onewire_new_bus_rmt(& bus_config, & rmt_config, & bus);
        if (err != ESP_OK)
        {
            PO_ERROR("onewire_new_bus_rmt");
            return false;
        }

        onewire_device_iter_handle_t iter = 0;
        onewire_device_t device;

        err = onewire_new_device_iter(bus, & iter);
        if (err != ESP_OK)
        {
            PO_ERROR("Error creating bus iterator");
            return false;
        }

        bool okay = true;
        while (true)
        {
            err = onewire_device_iter_get_next(iter, & device);
            if (err != ESP_OK)
                break;
            PO_DEBUG("found device %016llX", device.address);
            BusDevice *dev = new BusDevice(device.address);
            devices.push(dev, 0);
        }

        err = onewire_del_device_iter(iter);
        okay &= err == ESP_OK;
        return okay;
    }
};

    /*
     *
     */

OneWire *OneWire::create(int pin)
{
    PO_DEBUG("");
    OneWireRmt *dev = new OneWireRmt();

    if (dev->init(pin))
        return dev;

    delete dev;
    return 0;
}

bool init_onewire(Device *dev, void *arg)
{
    PO_DEBUG("");

    ASSERT(arg);
    struct DefOneWire *def = (struct DefOneWire*) arg;

    OneWire *onewire = OneWire::create(def->pin);
    if (onewire)
    {
        dev->add(Objects::objects, onewire);
    }

    return onewire;
}

}   //  panglos

#endif // defined(ONEWIRE_RMT)

//  FIN
