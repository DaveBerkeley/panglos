
#if defined(ESP32)

#include <stdint.h>

#include "driver/i2c_master.h"

#include "panglos/debug.h"

#include "panglos/esp32/hal.h"
#include "panglos/esp32/gpio.h"
#include "panglos/mutex.h"

#include "panglos/esp32/i2c.h"

namespace panglos {

#define esp_check(err) ASSERT_ERROR((err) == ESP_OK, "err=%s", lut(err_lut, (err)));

class ESP_I2C::I2cDevice
{
public:
    I2cDevice *next;
    i2c_master_dev_handle_t handle;
    uint8_t addr;

    I2cDevice(uint8_t a) : next(0), handle(0), addr(a) { }    

    static I2cDevice **get_next(I2cDevice *d) { return & d->next; }

    static int match(I2cDevice *d, void *arg)
    {
        ASSERT(arg);
        uint32_t addr = (uint32_t) arg;
        return d->addr = addr;
    }
};

    /*
     *
     */

ESP_I2C::ESP_I2C(int _chan, uint32_t _scl, uint32_t _sda, Mutex *mutex, bool _verbose)
:   I2C(mutex),
    handle(0),
    devices(ESP_I2C::I2cDevice::get_next),
    chan(_chan),
    scl(_scl),
    sda(_sda),
    verbose(_verbose)
{
    ESP_GPIO::mark_used(scl);
    ESP_GPIO::mark_used(sda);

    if (verbose) PO_DEBUG("scl=%d sda=%d", (int) scl, (int) sda);
 
    i2c_master_bus_config_t i2c_mst_config = {
        .i2c_port = -1,
        .sda_io_num = (gpio_num_t) sda,
        .scl_io_num = (gpio_num_t) scl,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
    };
    i2c_mst_config.flags.enable_internal_pullup = true;

    esp_err_t err = i2c_new_master_bus(& i2c_mst_config, (i2c_master_bus_handle_t*) & handle);
    esp_check(err);
}

ESP_I2C::~ESP_I2C()
{
    esp_err_t err = i2c_del_master_bus((i2c_master_bus_handle_t) handle);
    esp_check(err);
    ESP_GPIO::mark_unused(scl);
    ESP_GPIO::mark_unused(sda);

    while (!devices.empty())
    {
        I2cDevice *dev = devices.pop(mutex);
        ASSERT(dev);
        err = i2c_master_bus_rm_device((i2c_master_dev_handle_t) dev->handle);
        esp_check(err);
        delete dev;
    }
}

ESP_I2C::I2cDevice *ESP_I2C::get_device(uint8_t addr)
{
    uint32_t a = addr;
    I2cDevice *dev = devices.find(I2cDevice::match, (void*) a, mutex);

    if (!dev)
    {
        PO_DEBUG("create device addr=%#x", addr);
        dev = new I2cDevice(addr);
        devices.push(dev, mutex);

        i2c_device_config_t dev_cfg = {
            .dev_addr_length = I2C_ADDR_BIT_LEN_7,
            .device_address = addr,
            .scl_speed_hz = 100000,
        };
        esp_err_t err = i2c_master_bus_add_device(
                (i2c_master_bus_handle_t) handle, 
                & dev_cfg, 
                (i2c_master_dev_handle_t*) & dev->handle);
        esp_check(err);
    }

    return dev;
}

    /*
     *
     */

bool ESP_I2C::probe(uint8_t addr, uint32_t timeout)
{
    // timeout in ticks, ESPIDF API needs ms
    esp_err_t err = i2c_master_probe((i2c_master_bus_handle_t) handle, addr, timeout * 10);
    if (verbose) PO_DEBUG("addr=%#x err=%s", addr, lut(err_lut, (err)));
    return err == ESP_OK;
}

int ESP_I2C::write(uint8_t addr, const uint8_t* wr, uint32_t len)
{
    if (verbose) PO_DEBUG("addr=%#x", addr);
    I2cDevice *dev = get_device(addr);
    ASSERT(dev);
    esp_err_t err = i2c_master_transmit((i2c_master_dev_handle_t) dev->handle, wr, len, -1);
    return (err == ESP_OK) ? len : 0;
}

int ESP_I2C::write_read(uint8_t addr, const uint8_t* wr, uint32_t wr_len, uint8_t* rd, uint32_t rd_len)
{
    if (verbose) PO_DEBUG("addr=%#x", addr);
    I2cDevice *dev = get_device(addr);
    ASSERT(dev);
    esp_err_t err = i2c_master_transmit_receive(
            (i2c_master_dev_handle_t) dev->handle, 
            wr, wr_len, 
            rd, rd_len,
            -1);
    return (err == ESP_OK) ? rd_len : 0;
}

int ESP_I2C::read(uint8_t addr, uint8_t* rd, uint32_t len)
{
    if (verbose) PO_DEBUG("addr=%#x", addr);
    I2cDevice *dev = get_device(addr);
    ASSERT(dev);
    esp_err_t err = i2c_master_receive((i2c_master_dev_handle_t) dev->handle, rd, len, -1);
    return (err == ESP_OK) ? len : 0;
}

}   //  namespace panglos

#endif  //  ESP32

//  FIN
