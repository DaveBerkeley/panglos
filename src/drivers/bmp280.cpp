
#include <stdint.h>
#include <math.h>

#include "panglos/debug.h"

#include "panglos/mutex.h"
#include "panglos/drivers/i2c.h"
#include "panglos/drivers/bmp280.h"

namespace panglos {

#define ADDR 0x77

enum Reg {
    TEMP_XLSB = 0xfc,
    TEMP_LSB = 0xfb,
    TEMP_MSB = 0xfa,
    PRESS_XLSB = 0xf9,
    PRESS_LSB = 0xf8,
    PRESS_MSB = 0xf7,
    CONFIG = 0xf5,
    CTRL_MEAS = 0xf4,
    STATUS = 0xf3,
    RESET = 0xE0,
    ID = 0xD0,
    CALIB = 0x88,
};

    /*
     *  Yes the 16-data is { LSB, MSB, } and the 24-data
     *  really is { MSB, LSB, XLSB }.
     */

static uint16_t get_16(const uint8_t *data)
{
    //  LSB, MSB
    return data[0] + (data[1] << 8);
}

static uint32_t get_24(const uint8_t *data)
{
    // MSB, LSB, XLSB
    uint32_t r = data[0];
    r <<= 8;
    r += data[1];
    r <<= 4;
    r += data[2] >> 4;
    return r;
}

    /*
     *  Temp / pressure compensation algorithm adapted from datasheet.
     */

class BMP280::Cal
{
public:
    uint16_t dig_T1;
    int16_t dig_T2;
    int16_t dig_T3;

    uint16_t dig_P1;
    int16_t dig_P2;
    int16_t dig_P3;
    int16_t dig_P4;
    int16_t dig_P5;
    int16_t dig_P6;
    int16_t dig_P7;
    int16_t dig_P8;
    int16_t dig_P9;

    int32_t t_fine;

    // Returns temperature in DegC, resolution is 0.01 DegC. Output value of “5123” equals 51.23 DegC.
    // t_fine carries fine temperature as global value

    int32_t compensate_T(int32_t adc_T)
    {
        int32_t var1, var2, T;
        var1 = ((((adc_T>>3) - ((int32_t)dig_T1<<1))) * ((int32_t)dig_T2)) >> 11;
        var2 = (((((adc_T>>4) - ((int32_t)dig_T1)) * ((adc_T>>4) - ((int32_t)dig_T1)))
                >> 12) *
                ((int32_t)dig_T3)) >> 14;
        t_fine = var1 + var2;
        T = (t_fine * 5 + 128) >> 8;
        return T;
    }

    double to_t(int32_t adc)
    {
        return compensate_T(adc) / 100.0f;
    }

    // Returns pressure in Pa as unsigned 32 bit integer in Q24.8 format (24 integer bits and 8 fractional bits).
    // Output value of “24674867” represents 24674867/256 = 96386.2 Pa = 963.862 hPa

    uint32_t compensate_P(int32_t adc_P)
    {
        int64_t var1, var2, p;
        var1 = ((int64_t)t_fine) - 128000;
        var2 = var1 * var1 * (int64_t)dig_P6;
        var2 = var2 + ((var1*(int64_t)dig_P5)<<17);
        var2 = var2 + (((int64_t)dig_P4)<<35);
        var1 = ((var1 * var1 * (int64_t)dig_P3)>>8) + ((var1 * (int64_t)dig_P2)<<12);
        var1 = (((((int64_t)1)<<47)+var1))*((int64_t)dig_P1)>>33;
        if (var1 == 0)
        {
            return 0; // avoid exception caused by division by zero
        }
        p = 1048576-adc_P;
        p = (((p<<31)-var2)*3125)/var1;
        var1 = (((int64_t)dig_P9) * (p>>13) * (p>>13)) >> 25;
        var2 = (((int64_t)dig_P8) * p) >> 19;
        p = ((p + var1 + var2) >> 8) + (((int64_t)dig_P7)<<4);
        return (uint32_t)p;
    }

    double to_p(int32_t adc)
    {
        return compensate_P(adc) / 25600.0f;
    }

    bool read(uint8_t addr, I2C *i2c)
    {
        uint8_t cal[26];
        const uint8_t cmd = CALIB;

        int n = i2c->write_read(addr, & cmd, sizeof(cmd), cal, sizeof(cal));
        if (n != sizeof(cal))
        {
            PO_ERROR("reading cal n=%d", n);
            return false;
        }

        dig_T1 = get_16(& cal[0]);
        dig_T2 = get_16(& cal[2]);
        dig_T3 = get_16(& cal[4]);
        dig_P1 = get_16(& cal[6]);
        dig_P2 = get_16(& cal[8]);
        dig_P3 = get_16(& cal[10]);
        dig_P4 = get_16(& cal[12]);
        dig_P5 = get_16(& cal[14]);
        dig_P6 = get_16(& cal[16]);
        dig_P7 = get_16(& cal[18]);
        dig_P8 = get_16(& cal[20]);
        dig_P9 = get_16(& cal[22]);

        //PO_DEBUG("t1=%#x t2=%#x t3=%#x", dig_T1, dig_T2, dig_T3);
        //PO_DEBUG("p1=%#x p2=%#x p3=%#x", dig_P1, dig_P2, dig_P3);
        //PO_DEBUG("p4=%#x p5=%#x p6=%#x", dig_P4, dig_P5, dig_P6);
        //PO_DEBUG("p7=%#x p8=%#x p9=%#x", dig_P7, dig_P8, dig_P9);

        return true;
    }

};

    /*
     *
     */

BMP280::BMP280(I2C *_i2c)
:   i2c(_i2c),
    cal(0)
{
    cal = new Cal;
}

BMP280::~BMP280()
{
    delete cal;
}

bool BMP280::probe()
{
    uint8_t c = 0;
    const bool ok = i2c->write(ADDR, & c, sizeof(c)) == sizeof(c);
    return ok;
}

bool BMP280::init()
{
    const uint8_t id = get_id();
    if (id != 0x58)
    {
        PO_ERROR("bad id=%#x", id);
        return false;
    }

    struct Pair {
        uint8_t reg;
        uint8_t data;
    };
    const struct Pair pairs[] = {
        {   RESET, 0xb6, },
        {   CTRL_MEAS, 0x27, },
        {   CONFIG, 0x00, },
        {   0, },
    };

    for (const struct Pair *p = pairs; p->reg; p++)
    {
        const uint8_t cmd[] = { p->reg, p->data };
        const int n = i2c->write(ADDR, cmd, sizeof(cmd));
        if (n != sizeof(cmd))
        {
            PO_ERROR("writing reg=%#x", p->reg);
            return false;
        }
    }

    if (!cal->read(ADDR, i2c))
    {
        PO_ERROR("Error reading cal");
        return false;
    }

    return true;
}

bool BMP280::read(struct Data *data)
{
    ASSERT(data);
    const uint8_t cmd = { PRESS_MSB, };
    uint8_t regs[6];
    int n = i2c->write_read(ADDR, & cmd, sizeof(cmd), regs, sizeof(regs));
    uint32_t p_adc = get_24(& regs[0]);
    uint32_t t_adc = get_24(& regs[3]);
    data->temp = cal->to_t(t_adc);
    data->pressure = cal->to_p(p_adc);
    //PO_DEBUG("%#x %#x %f %f", p_adc, t_adc, p, t);
    return n == sizeof(data) + sizeof(cmd);
}

double BMP280::sealevel(double p, double alt)
{
	return(p/pow(1-(alt/44330.0),5.255));
}

uint8_t BMP280::get_id()
{
    const uint8_t cmd = { ID, };
    uint8_t c = 0;
    i2c->write_read(ADDR, & cmd, sizeof(cmd), & c, sizeof(c));
    return c;
}

}   //  namespace panglos

//  FIN
