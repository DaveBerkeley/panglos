
    /*
     *  ESP32 SPI
     */

#include "driver/spi_master.h"

#include "panglos/drivers/spi.h"

namespace panglos {

class ESP_SPI : public SPI
{
    spi_device_handle_t spi;
public:
    ESP_SPI(Mutex *m, int ck, int copi, int cipo, int cs=-1, int max_sz=32);
    ~ESP_SPI();

    virtual bool write(const uint8_t *data, int size) override;
    virtual bool io(const uint8_t *data, uint8_t *rd, int size) override;
};

}   //  namespace panglos


