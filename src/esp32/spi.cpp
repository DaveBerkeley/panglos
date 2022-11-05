
    /*
     *  ESP32 SPI : TOOO Move to separate file
     */

#include <string.h>

#include "panglos/debug.h"

#include "panglos/esp32/hal.h"

#include "panglos/esp32/spi.h"

namespace panglos {

ESP_SPI::ESP_SPI(Mutex *m, int ck, int copi, int cipo, int cs, int max_sz)
:   SPI(m),
    spi(0)
{
    esp_err_t err;

    spi_bus_config_t buscfg = {
        .mosi_io_num = copi,
        .miso_io_num = cipo,
        .sclk_io_num = ck,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = max_sz,
        //.flags = 0,
        //.intr_flags = 0,
    };
    spi_device_interface_config_t devcfg = {
        .mode=0,
        .clock_speed_hz=25000000,
        .spics_io_num=cs,
        .queue_size=7, // # transactions 
        //.pre_cb=0,
    };

    err = spi_bus_initialize(SPI2_HOST, & buscfg, SPI_DMA_CH_AUTO);
    esp_check(err, __LINE__);

    err = spi_bus_add_device(SPI2_HOST, & devcfg, & spi);
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
    ASSERT(0); // "Not implemented"
    return false;
}

}   //  namespace panglos

