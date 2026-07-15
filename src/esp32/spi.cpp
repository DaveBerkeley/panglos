
    /*
     *  ESP32 SPI : TOOO Move to separate file
     */

#include <string.h>

#include "panglos/debug.h"
#include "panglos/mutex.h"
#include "panglos/drivers/gpio.h"

#include "panglos/esp32/hal.h"
#include "panglos/esp32/spi.h"

namespace panglos {

    /*
     *
     */

static esp_err_t add_device(struct ESP_SPI::SPI_DEF *sd, int speed, spi_device_handle_t *handle)
{
    esp_err_t err;
    
    if (*handle)
    {
        err = spi_bus_remove_device(*handle);
        esp_check(err, __LINE__);
        if (err != ESP_OK) return err;
    } 

    spi_device_interface_config_t devcfg = {
        .mode=0,
        .clock_speed_hz=speed,
        .spics_io_num=sd->cs,
        .queue_size=7, // # transactions 
        //.pre_cb=0,
    };

    err = spi_bus_add_device(sd->dev, & devcfg, handle);
    esp_check(err, __LINE__);
    return err;
} 

    /*
     *
     */

ESP_SPI::ESP_SPI(Mutex *m, struct SPI_DEF *sd, int max_sz)
:   cs_lock(m),
    spi(0),
    speed(0)
{
    ASSERT(sd);
    memcpy(& def, sd, sizeof(def));

    PO_DEBUG("dev=%d", sd->dev);
    esp_err_t err;

    spi_bus_config_t buscfg = {
        .mosi_io_num = sd->copi,
        .miso_io_num = sd->cipo,
        .sclk_io_num = sd->ck,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = max_sz,
        //.flags = 0,
        //.intr_flags = 0,
    };

    err = spi_bus_initialize(sd->dev, & buscfg, SPI_DMA_CH_AUTO);
    esp_check(err, __LINE__);

    speed = sd->clock_hz ? sd->clock_hz : 12000000;
    err = add_device(sd, speed, & spi);
    esp_check(err, __LINE__);
}

ESP_SPI::~ESP_SPI()
{
    if (spi)
    {
        spi_bus_remove_device(spi);
    }
}

bool ESP_SPI::read(uint8_t *data, int size)
{
    PO_ERROR("TODO");
    IGNORE(data);
    IGNORE(size);
    ASSERT(0);
    return false;
}

bool ESP_SPI::write(const uint8_t *data, int size)
{
    ASSERT(data);
    ASSERT(size);

    spi_transaction_t t;
    memset(&t, 0, sizeof(t));
    t.length = 8 * size;
    t.tx_buffer = data;
    t.user = 0;
    t.flags = 0; // SPI_TRANS_USE_TXDATA;
    esp_err_t err = spi_device_polling_transmit(spi, & t);
    esp_check(err, __LINE__);
    return err == ESP_OK;
}

bool ESP_SPI::io(const uint8_t *data, uint8_t *rd, int size)
{
    ASSERT(data);
    ASSERT(rd);
    ASSERT(size);

    spi_transaction_t t;
    memset(&t, 0, sizeof(t));
    t.length = 8 * size;
    t.tx_buffer = data;
    t.rxlength = 8 * size;
    t.rx_buffer = rd;
    t.user = 0;
    t.flags = 0; // SPI_TRANS_USE_TXDATA;
    //t.override_freq_hz = override_freq_hz;
    esp_err_t err = spi_device_polling_transmit(spi, & t);
    esp_check(err, __LINE__);
    return err == ESP_OK;
}

void ESP_SPI::set_blocking(bool blocking)
{
    PO_ERROR("TODO blocking=%d", blocking);
}

uint32_t ESP_SPI::set_speed(uint32_t hz)
{
    if (hz == speed) return hz;

    esp_err_t err = add_device(& def, hz, & spi);
    esp_check(err, __LINE__);
    speed = hz;
    
    int freq_khz = 0;
    err = spi_device_get_actual_freq(spi, & freq_khz);
    esp_check(err, __LINE__);
    if (err != ESP_OK) return 0;
 
    PO_ERROR("req_hz=%d actual=%d", hz, freq_khz * 1000);
    return freq_khz * 1000;
}

SPI::Lock *ESP_SPI::lock()
{
    return & cs_lock;
}

}   //  namespace panglos

