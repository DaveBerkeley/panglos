
    /*
     *  ESP32 SPI
     */

#pragma once

#include "driver/spi_master.h"
#include "hal/spi_types.h"
#include "hal/gpio_types.h"

#include "panglos/drivers/spi.h"

namespace panglos {

    /*
     *
     */

class ESP_SPI : public SPI
{
public:
    struct SPI_DEF
    {
        spi_host_device_t dev;
        gpio_num_t ck;
        gpio_num_t copi;
        gpio_num_t cipo;
        gpio_num_t cs;
        int clock_hz;
    };
 
private:
    struct SPI_DEF def;
    SPI::Lock cs_lock;
    spi_device_handle_t spi;
    int speed;

public:
    ESP_SPI(Mutex *m, struct SPI_DEF *sd, int max_sz=32);
    ~ESP_SPI();

    virtual bool read(uint8_t *data, int size) override;
    virtual bool write(const uint8_t *data, int size) override;
    virtual bool io(const uint8_t *data, uint8_t *rd, int size) override;

    virtual void set_blocking(bool blocking) override;
    virtual uint32_t set_speed(uint32_t hz) override;

    virtual Lock *lock() override;
};

}   //  namespace panglos


