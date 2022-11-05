
    /*
     *  Round OLED
     */

namespace panglos {

class SPI;
class GPIO;

class OLED
{
    SPI *spi;
    GPIO *bl;
    GPIO *dc;
    GPIO *re;
    GPIO *cs;

    int width;
    int height;

    void send(uint8_t command, const uint8_t *data, uint8_t n);

public:
    OLED(SPI *_spi, GPIO *_cs, GPIO *_backlight, GPIO *_control, GPIO *_reset);

    void init();
    void backlight(bool on);

    void reset();
    void set_addr_window(uint16_t x1, uint16_t y1, uint16_t w, uint16_t h);
    void set_rotation(int n);
    void invert(bool inv);

    void write(const uint8_t *data, size_t size);
};

}   //  namespace panglos

//  FIN
