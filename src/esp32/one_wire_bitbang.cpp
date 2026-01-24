
#if defined(ONEWIRE_BITBANG)

#include <string.h>

#include "onewire.h"

#include "panglos/debug.h"
#include "panglos/list.h"
#include "panglos/device.h"
#include "panglos/object.h"

#include "panglos/drivers/one_wire.h"

namespace panglos {


class OneWireBitBang : public _OneWire
{
    gpio_num_t pin;

    virtual bool reset() override
    {
        return onewire_reset(pin);
    }

    virtual bool write(const uint8_t *buff, size_t s) override
    {
        return onewire_write_bytes(pin, buff, s);
    }

    virtual bool read(uint8_t *buff, size_t s) override
    {
        return onewire_read_bytes(pin, buff, s);
    }

public:
    OneWireBitBang()
    {
    }

    ~OneWireBitBang()
    {
        while (!devices.empty())
        {
            BusDevice *dev = devices.pop(0);
            delete dev;
        }
    }

    bool init(int p)
    {
        pin = (gpio_num_t) p;

        gpio_config_t config = {
            .pin_bit_mask = (uint64_t) 1L << pin,
            .mode = GPIO_MODE_INPUT_OUTPUT_OD,
            .pull_up_en  = GPIO_PULLUP_ENABLE,
            .pull_down_en = GPIO_PULLDOWN_DISABLE,
            .intr_type = GPIO_INTR_DISABLE,
        };
        
        esp_err_t err = gpio_config(& config);
        if (err != ESP_OK)
        {
            PO_ERROR("initialising GPIO(%d)", pin);
            return false;
        }

        if (!reset())
        {
            PO_DEBUG("no onewire devices found");
            return false;
        }

        onewire_search_t search_state;
        onewire_search_start(& search_state);
        uint64_t addr;

        while (true)
        {
            onewire_addr_t a = onewire_search_next(& search_state, pin);
            if (a == ONEWIRE_NONE) break;
            PO_DEBUG("found addr %llx", a);
            BusDevice *dev = new BusDevice(a);
            devices.push(dev, 0);
        }

        return true;
    }

};

    /*
     *
     */

OneWire *OneWire::create_bitbang(int pin)
{
    OneWireBitBang *ow = new OneWireBitBang();

    if (ow->init(pin))
        return ow;
    
    delete ow;
    PO_ERROR("OneWireBitBang::init()");
    return 0;
}

bool init_onewire_bitbang(Device *dev, void *arg)
{
    PO_DEBUG("");

    ASSERT(arg);
    struct DefOneWire *def = (struct DefOneWire*) arg;

    OneWire *ow = OneWire::create_bitbang(def->pin);

    if (!ow)
    {
        PO_ERROR("");
        return false;
    }

    Objects::objects->add(dev->name, ow);

    return ow;
}

}   //  panglos

#endif // ONEWIRE_BITBANG

//  FIN
