
    /*
     *  ESP32 SPI : TOOO Move to separate file
     */

#include <string.h>

#include "panglos/debug.h"

#include "panglos/esp32/hal.h"

#include "panglos/esp32/spi.h"

namespace panglos {

ESP_SPI::ESP_SPI(Mutex *m, struct SPI_DEF *sd, int max_sz)
:   SPI(m),
    spi(0)
{
    ASSERT(sd);
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
    spi_device_interface_config_t devcfg = {
        .mode=0,
        //.clock_speed_hz=25000000,
        .clock_speed_hz= sd->clock_hz ? sd->clock_hz : 12000000,
        .spics_io_num=sd->cs,
        .queue_size=7, // # transactions 
        //.pre_cb=0,
    };

    err = spi_bus_initialize(sd->dev, & buscfg, SPI_DMA_CH_AUTO);
    esp_check(err, __LINE__);

    err = spi_bus_add_device(sd->dev, & devcfg, & spi);
    esp_check(err, __LINE__);
    ASSERT(spi);
}

ESP_SPI::~ESP_SPI()
{
    if (spi)
    {
        spi_bus_remove_device(spi);
    }
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
    esp_err_t err = spi_device_polling_transmit(spi, & t);
    esp_check(err, __LINE__);
    return err == ESP_OK;
}

}   //  namespace panglos

