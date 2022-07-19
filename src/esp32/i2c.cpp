
#if defined(ESP32)

#include <stdint.h>

#include "driver/i2c.h"

#include "panglos/debug.h"

#include "panglos/esp32/hal.h"
#include "panglos/esp32/gpio.h"
#include "panglos/mutex.h"

#include "panglos/esp32/i2c.h"

namespace panglos {

#define esp_check(err) ASSERT_ERROR((err) == ESP_OK, "err=%s", lut(err_lut, (err)));

ESP_I2C::ESP_I2C(int _chan, uint32_t _scl, uint32_t _sda, Mutex *mutex, bool _verbose)
:   I2C(mutex),
    chan(_chan),
    scl(_scl),
    sda(_sda),
    verbose(_verbose)
{
    ESP_GPIO::mark_used(scl);
    ESP_GPIO::mark_used(sda);

    if (verbose) PO_DEBUG("scl=%d sda=%d", scl, sda);
 
    i2c_config_t config = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = (int) sda,
        .scl_io_num = (int) scl,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master = {
            .clk_speed = 100000,
        },
        .clk_flags = 0,
    };

    esp_err_t err = i2c_param_config(chan, & config);
    esp_check(err);

    const size_t buff_size = 1024;
    const int intr_alloc_flags = 0;
    err = i2c_driver_install(chan, I2C_MODE_MASTER, buff_size, buff_size, intr_alloc_flags);
    esp_check(err);
}

ESP_I2C::~ESP_I2C()
{
    esp_err_t err = i2c_driver_delete(chan);
    esp_check(err);
    ESP_GPIO::mark_used(scl);
    ESP_GPIO::mark_used(sda);
}

    /*
     *
     */

#define WR(addr) ((addr) << 1)
#define RD(addr) (1 | ((addr) << 1))

    /*
     *
     */

class Transaction
{
    i2c_cmd_handle_t h;
public:
    Transaction()
    :   h(0)
    {
        h = i2c_cmd_link_create();
    }

    void start()
    {
        esp_err_t err = i2c_master_start(h);
        esp_check(err);
    }

    void write(const uint8_t *data, int len)
    {
        esp_err_t err = i2c_master_write(h, data, len, true);
        esp_check(err);
    }

    void write(uint8_t data)
    {
        esp_err_t err = i2c_master_write_byte(h, data, true);
        esp_check(err);
    }

    void read(uint8_t *data, int len, bool last)
    {
        i2c_ack_type_t ack = last ? I2C_MASTER_LAST_NACK : I2C_MASTER_NACK;
        esp_err_t err = i2c_master_read(h, data, len, ack);
        esp_check(err);
    }

    void stop()
    {
        esp_err_t err = i2c_master_stop(h);
        esp_check(err);
    }

    bool send(int port, int timeout)
    {
        esp_err_t err = i2c_master_cmd_begin(port, h, timeout);
        return err == ESP_OK;
    }

    ~Transaction()
    {
        i2c_cmd_link_delete(h);
    }
};

    /*
     *
     */

bool ESP_I2C::probe(uint8_t addr, uint32_t timeout)
{
    Transaction io;

    io.start();
    io.write(RD(addr));
    io.stop();
    bool ok = io.send(chan, timeout);

    if (verbose) PO_DEBUG("addr=%#x ack=%d", addr, ok);

    return ok;
}

int ESP_I2C::write(uint8_t addr, const uint8_t* wr, uint32_t len)
{
    if (verbose) PO_DEBUG("addr=%#x", addr);

    Transaction io;

    io.start();
    io.write(WR(addr));
    io.write(wr, len);
    io.stop();
    bool ok = io.send(chan, 10);
    return ok ? len : 0;
}

int ESP_I2C::write_read(uint8_t addr, const uint8_t* wr, uint32_t wr_len, uint8_t* rd, uint32_t rd_len)
{
    if (verbose) PO_DEBUG("addr=%#x", addr);

    Transaction io;

    io.start();
    io.write(WR(addr));
    io.write(wr, wr_len);
    io.start();
    io.write(RD(addr));
    io.read(rd, rd_len, true);
    io.stop();
    bool ok = io.send(chan, 10);
    return ok ? rd_len : 0;
}

int ESP_I2C::read(uint8_t addr, uint8_t* rd, uint32_t len)
{
    if (verbose) PO_DEBUG("addr=%#x", addr);

    Transaction io;

    io.start();
    io.write(RD(addr));
    io.read(rd, len, true);
    io.stop();
    bool ok = io.send(chan, 10);
    return ok ? len : 0;
}

}   //  namespace panglos

#endif  //  ESP32

//  FIN
