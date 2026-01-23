
#include <string.h>

#include "onewire_bus.h"
#include "ds18b20.h"

#include "panglos/debug.h"
#include "panglos/time.h"
#include "panglos/list.h"
#include "panglos/device.h"
#include "panglos/object.h"

#include "panglos/drivers/one_wire.h"

namespace panglos {

    /*
     *
     */

class OneWireDevice
{
    OneWireDevice *next;
public:
    onewire_bus_handle_t bus;
    onewire_device_t device;
    bool allocated;

    OneWireDevice(onewire_bus_handle_t b, onewire_device_t d)
    :   next(0),
        bus(b),
        device(d),
        allocated(false)
    {
    }
    static OneWireDevice **get_next(OneWireDevice *dev) { return & dev->next; }
};

    /*
     *  The ESPIDF library has OS vTaskDelay() calls embedded in the API.
     *  It assumes that you are reading in a thread and that blocking
     *  the thread for up to 800ms is fine.
     *
     *  So I've cut and pasted the send_command code, removing the delay.
     *  The timing of conversion requests is handled in the DS18B20_Sensor class.
     */

#define ONEWIRE_CMD_MATCH_ROM       0x55

#define DS18B20_CMD_CONVERT_TEMP    0x44

static esp_err_t ds18b20_send_command(OneWireDevice *dev, uint8_t cmd)
{
    // send : { ONEWIRE_CMD_MATCH_ROM, addr[8], cmd }
    const size_t sz = sizeof(dev->device.address);
    uint8_t tx_buffer[sz + 2];
    int idx = 0;

    tx_buffer[idx++] = ONEWIRE_CMD_MATCH_ROM;
    memcpy(& tx_buffer[idx], & dev->device.address, sz);
    idx += sz;
    tx_buffer[idx] = cmd;

    return onewire_bus_write_bytes(dev->bus, tx_buffer, sizeof(tx_buffer));
}

    /*
     *
     */

class DS18B20_Sensor : public TemperatureSensor
{
    OneWireDevice *onewire;
    ds18b20_device_handle_t handle;
    float temperature;
    Time::tick_t requested;

    bool start_next_conversion()
    {
        esp_err_t err = onewire_bus_reset(onewire->bus);
        if (err != ESP_OK) return false;
        err = ds18b20_send_command(onewire, DS18B20_CMD_CONVERT_TEMP);

        requested = Time::get();

        return err == ESP_OK;
    }

    virtual bool get_temp(double *t) override
    {
        bool okay = true;

        // conversion takes up to 750ms
        if (Time::elapsed(requested, 80))
        {
            esp_err_t err = ds18b20_get_temperature(handle, & temperature);
            if (err != ESP_OK) return false;

            okay = start_next_conversion();
        }

        if (t) *t = temperature;
        return okay;
    }

public:
    DS18B20_Sensor(OneWireDevice *ow, ds18b20_device_handle_t h)
    :   onewire(ow),
        handle(h),
        temperature(85.0),
        requested(0)
    {
        PO_DEBUG("");
        esp_err_t err = ds18b20_set_resolution(handle, DS18B20_RESOLUTION_12B);
        ASSERT(err == ESP_OK);
    }

    ~DS18B20_Sensor()
    {
        esp_err_t err = ds18b20_del_device(handle);
        ASSERT(err == ESP_OK);
    }
};

    /*
     *
     */

class _OneWire : public OneWire
{
public:
    onewire_bus_handle_t bus;

    panglos::List<OneWireDevice*> devices;

    _OneWire(int pin)
    :   devices(OneWireDevice::get_next)
    {
        PO_DEBUG("");

        onewire_bus_config_t bus_config = {
            .bus_gpio_num = pin,
            .flags = {
                .en_pull_up = true, // enable the internal pull-up resistor in case the external device didn't have one
            }
        };
        onewire_bus_rmt_config_t rmt_config = {
            .max_rx_bytes = 10, // 1byte ROM command + 8byte ROM number + 1byte device command
        };

        // create the bus
        esp_err_t err = onewire_new_bus_rmt(& bus_config, & rmt_config, & bus);
        ASSERT(err == ESP_OK);

        onewire_device_iter_handle_t iter = 0;
        onewire_device_t device;

        err = onewire_new_device_iter(bus, & iter);
        ASSERT(err == ESP_OK);
        while (true)
        {
            err = onewire_device_iter_get_next(iter, & device);
            if (err != ESP_OK) break;
            PO_DEBUG("found device %016llX", device.address);
            OneWireDevice *dev = new OneWireDevice(bus, device);
            devices.push(dev, 0);
        }

        err = onewire_del_device_iter(iter);
        ASSERT(err == ESP_OK);
    }

    ~_OneWire()
    {
        while (!devices.empty())
        {
            OneWireDevice *dev = devices.pop(0);
            delete dev;
        }
    }

    virtual DS18B20_Sensor *get_ds18b20() override
    {
        for (OneWireDevice *dev = devices.head; dev; dev = *OneWireDevice::get_next(dev))
        {
            if (dev->allocated) continue;

            ds18b20_config_t ds_cfg = {};
            ds18b20_device_handle_t handle;
            esp_err_t err = ds18b20_new_device_from_enumeration(& dev->device, & ds_cfg, & handle);
            if (err != ESP_OK)
            {
                continue;
            }
            dev->allocated = true;
            return new DS18B20_Sensor(dev, handle);
        }
        return 0;
    }
};

    /*
     *
     */

OneWire *OneWire::create(int pin)
{
    return new _OneWire(pin);
}

bool init_onewire(Device *dev, void *arg)
{
    PO_DEBUG("");

    ASSERT(arg);
    struct DefOneWire *def = (struct DefOneWire*) arg;

    OneWire *onewire = OneWire::create(def->pin);
    dev->add(Objects::objects, onewire);
    return true;
}

}   //  panglos

//  FIN
