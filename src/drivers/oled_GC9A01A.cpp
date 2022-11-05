
#include <string.h>

#include "panglos/debug.h"

#include "panglos/time.h"
#include "panglos/mutex.h"

#include "panglos/drivers/gpio.h"
#include "panglos/drivers/spi.h"

#include "panglos/drivers/oled.h"

    /*
     *  OLED SPI Driver for the GC9A01A
     */

namespace panglos {

//  https://github.com/PaintYourDragon/Adafruit_GC9A01A/blob/master/Adafruit_GC9A01A.h

#define MADCTL_MX 0x40  ///< Right to left
#define MADCTL_BGR 0x08 ///< Blue-Green-Red pixel order
#define MADCTL_MY 0x80  ///< Bottom to top
#define MADCTL_MV 0x20  ///< Reverse Mode

#define GC9A01A_INVON 0x21    ///< Display Inversion ON
#define GC9A01A_INVOFF 0x20   ///< Display Inversion OFF

#define GC9A01A_INREGEN2 0xEF ///< Inter register enable 2
#define GC9A01A_INREGEN1 0xFE ///< Inter register enable 1

#define GC9A01A_MADCTL 0x36   ///< Memory Access Control
#define GC9A01A_PIXFMT 0x3A   ///< COLMOD: Pixel Format Set

#define GC9A01A1_VREG1A 0xC3 ///< Vreg1a voltage control
#define GC9A01A1_VREG1B 0xC4 ///< Vreg1b voltage control
#define GC9A01A1_VREG2A 0xC9 ///< Vreg2a voltage control

#define ILI9341_GMCTRN1 0xE1 ///< Negative Gamma Correction

#define GC9A01A_GAMMA1 0xF0 ///< Set gamma 1
#define GC9A01A_GAMMA2 0xF1 ///< Set gamma 2
#define GC9A01A_GAMMA3 0xF2 ///< Set gamma 3
#define GC9A01A_GAMMA4 0xF3 ///< Set gamma 4

#define ILI9341_FRAMERATE 0xE8 ///< Frame rate control

#define GC9A01A_TEON 0x35     ///< Tearing effect line on
#define GC9A01A_INVON 0x21    ///< Display Inversion ON
#define GC9A01A_SLPOUT 0x11 ///< Sleep Out
#define GC9A01A_DISPON 0x29   ///< Display ON
#define GC9A01A_CASET 0x2A ///< Column Address Set
#define GC9A01A_PASET 0x2B ///< Page Address Set
#define GC9A01A_RAMWR 0x2C ///< Memory Write

    /*
     *
     */

static const uint8_t initcmd[] = {
  GC9A01A_INREGEN2, 0,
  0xEB, 1, 0x14,
  GC9A01A_INREGEN1, 0,
  GC9A01A_INREGEN2, 0,
  0xEB, 1, 0x14,
  0x84, 1, 0x40,
  0x85, 1, 0xFF,
  0x86, 1, 0xFF,
  0x87, 1, 0xFF,
  0x88, 1, 0x0A,
  0x89, 1, 0x21,
  0x8A, 1, 0x00,
  0x8B, 1, 0x80,
  0x8C, 1, 0x01,
  0x8D, 1, 0x01,
  0x8E, 1, 0xFF,
  0x8F, 1, 0xFF,
  0xB6, 2, 0x00, 0x20,
  GC9A01A_MADCTL, 1, MADCTL_BGR,
  GC9A01A_PIXFMT, 1, 0x05,
  0x90, 4, 0x08, 0x08, 0x08, 0x08,
  0xBD, 1, 0x06,
  0xBC, 1, 0x00,
  0xFF, 3, 0x60, 0x01, 0x04,
  GC9A01A1_VREG1A, 1, 0x13,
  GC9A01A1_VREG1B, 1, 0x13,
  GC9A01A1_VREG2A, 1, 0x22,
  0xBE, 1, 0x11,
  ILI9341_GMCTRN1, 2, 0x10, 0x0E,
  0xDF, 3, 0x21, 0x0c, 0x02,
  GC9A01A_GAMMA1, 6, 0x45, 0x09, 0x08, 0x08, 0x26, 0x2A,
  GC9A01A_GAMMA2, 6, 0x43, 0x70, 0x72, 0x36, 0x37, 0x6F,
  GC9A01A_GAMMA3, 6, 0x45, 0x09, 0x08, 0x08, 0x26, 0x2A,
  GC9A01A_GAMMA4, 6, 0x43, 0x70, 0x72, 0x36, 0x37, 0x6F,
  0xED, 2, 0x1B, 0x0B,
  0xAE, 1, 0x77,
  0xCD, 1, 0x63,
  0x70, 9, 0x07, 0x07, 0x04, 0x0E, 0x0F, 0x09, 0x07, 0x08, 0x03,
  ILI9341_FRAMERATE, 1, 0x34,
  0x62, 12, 0x18, 0x0D, 0x71, 0xED, 0x70, 0x70,
            0x18, 0x0F, 0x71, 0xEF, 0x70, 0x70,
  0x63, 12, 0x18, 0x11, 0x71, 0xF1, 0x70, 0x70,
            0x18, 0x13, 0x71, 0xF3, 0x70, 0x70,
  0x64, 7, 0x28, 0x29, 0xF1, 0x01, 0xF1, 0x00, 0x07,
  0x66, 10, 0x3C, 0x00, 0xCD, 0x67, 0x45, 0x45, 0x10, 0x00, 0x00, 0x00,
  0x67, 10, 0x00, 0x3C, 0x00, 0x00, 0x00, 0x01, 0x54, 0x10, 0x32, 0x98,
  0x74, 7, 0x10, 0x85, 0x80, 0x00, 0x00, 0x4E, 0x00,
  0x98, 2, 0x3e, 0x07,
  GC9A01A_TEON, 0,
  GC9A01A_INVON, 0,

  //GC9A01A_SLPOUT, 1, 0x80, // Exit sleep
  // delay 120
  //GC9A01A_DISPON, 1, 0x80, // Display on
  // delay 20
  0x00                  // End of list
};

    /*
     *  Round OLED
     */

OLED::OLED(SPI *_spi, GPIO *_cs, GPIO *_backlight, GPIO *_control, GPIO *_reset)
:   spi(_spi),
    bl(_backlight),
    dc(_control),
    re(_reset),
    cs(_cs)
{
}

void OLED::backlight(bool on)
{
    bl->set(on);
}

void OLED::send(uint8_t command, const uint8_t *data, uint8_t n)
{
    Lock lock(spi->mutex);

    cs->set(1);

    dc->set(0);
    cs->set(0);

    spi->write(& command, 1);

    dc->set(1);
    if (n)
    {
        spi->write(data, n);
    }
    cs->set(1);
    
    cs->set(0);
}

void OLED::init()
{
    reset();

    const uint8_t *addr = initcmd;

    while (*addr)
    {
        const uint8_t cmd = *addr++;
        const uint8_t n = *addr++;
        send(cmd, addr, n);
        addr += n;
    }

    uint8_t d[] = { 0x80 };
    send(GC9A01A_SLPOUT, d, sizeof(d)); // exit sleep
    // must wait 5ms before the next command
    // wait 120ms before sending next SleepIn command
    Time::msleep(5);
    //Time::msleep(120);
    send(GC9A01A_DISPON, d, sizeof(d)); // display on
    //Time::msleep(20);

    set_rotation(0);
}


void OLED::reset()
{
    ASSERT(re);
    re->set(1);
    Time::msleep(150);
    re->set(0);
    Time::msleep(150);
    re->set(1);
    Time::msleep(150);
}

void OLED::write(const uint8_t *data, size_t size)
{
    Lock lock(spi->mutex);

    dc->set(1);
    cs->set(0);

    while (size)
    {
        size_t block = 64;
        if (size < block)
        {
            block = size;
        }

        spi->write(data, block);

        data += block;
        size -= block;
    }

    cs->set(1);
    cs->set(0);
}

void OLED::set_addr_window(uint16_t x1, uint16_t y1, uint16_t w, uint16_t h)
{
    uint16_t x2 = x1 + w - 1;
    uint16_t y2 = y1 + h - 1;

    uint8_t col[] = { uint8_t(x1>>8), uint8_t(x1), uint8_t(x2>>8), uint8_t(x2), };
    uint8_t row[] = { uint8_t(y1>>8), uint8_t(y1), uint8_t(y2>>8), uint8_t(y2), };

    send(GC9A01A_CASET, col, sizeof(col));
    send(GC9A01A_PASET, row, sizeof(row));
    send(GC9A01A_RAMWR, 0, 0);
}

void OLED::set_rotation(int n)
{
    ASSERT((n >= 0) && (n < 4));
    uint8_t data = 0;

    switch (n)
    {
        case 0 :
        {
            data = MADCTL_BGR;
            width = 240;
            height = 240;
            break;
        }
        case 1 :
        {
            data = MADCTL_MX | MADCTL_MV | MADCTL_BGR;
            width = 240;
            height = 240;
            break;
        }
        case 2 :
        {
            data = MADCTL_MX | MADCTL_MY | MADCTL_BGR;
            width = 240;
            height = 240;
            break;
        }
        case 3 :
        {
            data = MADCTL_MY | MADCTL_MV | MADCTL_BGR;
            width = 240;
            height = 240;
            break;
        }
    }
    send(GC9A01A_MADCTL, & data, 1);
}

void OLED::invert(bool inv)
{
    send(inv ? GC9A01A_INVON : GC9A01A_INVOFF, 0, 0);
}

}   //  namespace panglos

//  FIN
