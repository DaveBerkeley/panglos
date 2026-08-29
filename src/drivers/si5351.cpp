
#include <math.h>

#include "panglos/debug.h"

#include "panglos/mutex.h"
#include "panglos/time.h"
#include "panglos/drivers/i2c.h"
#include "panglos/drivers/si5351.h"

namespace panglos {

#if 0

Xtal - 25,27Mhz
ext clock : 10Mh .. 100MHz
PLL output 600MHz .. 900MHz
Clock output 2.5kHz .. 200MHz

#endif

    /*
     *
     */

enum Regs {
    Device_Status = 0,
    Output_Enable_Control = 3, // 6 regs
    OEB_Enable_Control_Mask = 9,
    PLL_Input_Source = 15,
    CLK_Control = 16, // 8 regs
    CLK_Disable_State = 24, // 2 regs
    Multisynth_N_Params = 26, // 8 regs, * 2 PLLs
    Multisynth_X_Params = 42, // 8 regs, * 8 Ms
    CLK_6_7_Divider = 92, // 
    Spread_Spectrum_Parameters = 149,
    VCXO_Parameter = 162, // 3 regs
    CLK_Initial_Phase_Offset = 165, // 6 regs
    PLL_Reset = 177,
    Crystal_Internal_Load_Capacitance = 183,
    Fanout_Enable = 187,
};

    /*
     *
     */

void Si5351::set_p123(uint8_t *data, struct P3 *p)
{
    ASSERT(p);
    data[0] = uint8_t(p->p3 >> 8);
    data[1] = uint8_t(p->p3);
    data[2] = uint8_t(p->p1 >> 16);
    data[3] = uint8_t(p->p1 >> 8);
    data[4] = uint8_t(p->p1);
    data[5] = ((p->p2 >> 16) & 0x0f) + (uint8_t(p->p3 >> 16) << 4);
    data[6] = uint8_t(p->p2 >> 8);
    data[7] = uint8_t(p->p2);
}

bool Si5351::write_pll(uint8_t pll, struct P3 *p)
{
    ASSERT((pll == 0) || (pll == 1));
    ASSERT(p);
    uint8_t wr[9];

    wr[0] = (pll == 0) ? Multisynth_N_Params : (Multisynth_N_Params + 8);
    set_p123(& wr[1], p);

    PO_DEBUG("p1=0x%x p2=0x%x p3=0x%x", (int) p->p1, (int) p->p2, (int) p->p3);
    PO_DEBUG("reg=%d %02x %02x %02x %02x %02x %02x %02x %02x",
            (int) wr[0],
            (int) wr[1], (int) wr[2], (int) wr[3], (int) wr[4],
            (int) wr[5], (int) wr[6], (int) wr[7], (int) wr[8]);

    return i2c->write(addr, wr, sizeof(wr));
}

bool Si5351::write_msx(uint8_t chan, struct P3 *p, uint8_t op_div)
{
    ASSERT(chan < 8);
    uint8_t wr[9];

    wr[0] = Multisynth_X_Params + (8 * chan);
    set_p123(& wr[1], p);

    uint8_t div = 0;
    bool en_4 = false;
    switch (op_div)
    {
        // op_div MUST be a non-zero power of 2
        case 1:   div = 0; break;
        case 2:   div = 1; break;
        case 4:   div = 2; break;
        case 8:   div = 3; break;
        case 16:  div = 4; break;
        case 32:  div = 5; break;
        case 64:  div = 6; break;
        case 128: div = 7; break;
        default : ASSERT(0);
    }

    wr[3] += (div << 4) + (en_4 ? 0x0c : 0);

    PO_DEBUG("p1=0x%x p2=0x%x p3=0x%x", (int) p->p1, (int) p->p2, (int) p->p3);
    PO_DEBUG("reg=%d %02x %02x %02x %02x %02x %02x %02x %02x",
            (int) wr[0],
            (int) wr[1], (int) wr[2], (int) wr[3], (int) wr[4],
            (int) wr[5], (int) wr[6], (int) wr[7], (int) wr[8]);

    return i2c->write(addr, wr, sizeof(wr));
}

void Si5351::abc_to_p3(struct P3 *p, uint32_t a, uint32_t b, uint32_t c)
{
    p->p1 = (128 * a) + floor(128 * b / c) - 512;
    p->p2 = (128 * b) - (c * floor(128 * b / c));
    p->p3 = c;
}

    /*
     *
     */

bool Si5351::loss_of_lock(uint8_t pll)
{
    ASSERT((pll == 0) || (pll == 1));
    uint8_t reg = Device_Status;
    uint8_t data;

    if (!i2c->write_read(addr, & reg, sizeof(reg), & data, sizeof(data)))
    {
        PO_ERROR("reading Si5351 Device_Status");
        return false;
    }

    uint8_t mask = (pll == 0) ? 0x20 : 0x40;
    return data & mask;
}

    /*
     *
     */

Si5351::Si5351(I2C *_i2c, uint8_t _addr)
:   i2c(_i2c),
    addr(_addr ? _addr : ADDR),
    xtal_freq(0)
{
    PO_DEBUG("");
    ASSERT(i2c);
}

    /*
     *
     */

bool Si5351::init(uint32_t ms)
{
    uint8_t reg = Device_Status;
    uint8_t data = 0;

    Time::tick_t now = Time::get();

    while (true)
    {
        if (!i2c->write_read(addr, & reg, sizeof(reg), & data, sizeof(data)))
        {
            PO_ERROR("");
            return false;
        }

        if (!(data & 0x80)) break; // check SYS_INIT flag

        if (Time::elapsed(now, ms))
        {
            PO_ERROR("Timeout waiting for SYS_INIT");
            return false;
        }

        Time::msleep(5);
    }

    for (uint8_t pll = 0; pll < 2; pll++)
    {
        if (loss_of_lock(pll))
        {
            PO_DEBUG("loss_of_lock pll=%d", pll);
        }
    }

    // REVID shows which silicon the device is A=0 ? others?
    //uint8_t revision = data & 0x03;
    return true;
}

void Si5351::set_xtal(XtalConfig *config)
{
    PO_DEBUG("freq=%d", (int) config->freq);
    xtal_freq = config->freq;
}

bool Si5351::reset_pll(bool a, bool b)
{
    uint8_t mask = 0;
    if (a) mask |= 0x20;
    if (b) mask |= 0x80;

    PO_DEBUG("a=%d b=%d", a, b);
    uint8_t wr[2] { PLL_Reset, mask };
    return i2c->write(addr, wr, sizeof(wr));
}

bool Si5351::set_pll(uint8_t pll, PllConfig *config)
{
    ASSERT((pll == 0) || (pll == 1));

    PO_DEBUG("a=%d b=%d c=%d", (int) config->a, (int) config->b, (int) config->c);

    struct P3 p;
    abc_to_p3(& p, config->a, config->b, config->c);
    if (!write_pll(pll, & p))
        return false;
 
    // TODO set the PLL clock source : default is XTAL for both PLLs
    // TODO clock in div for ext clock

    return true;
}

bool Si5351::set_clock(uint8_t chan, ClockConfig *config)
{
    ASSERT(chan < 8);

    uint8_t data[2] = { uint8_t(CLK_Control + chan), };

    switch (config->state)
    {
        case ClockConfig::HI :
        case ClockConfig::LO :   ASSERT(0);
        case ClockConfig::OFF :
        {
            PO_DEBUG("clock=%d Off", chan);
            data[1] |= 0x80; 
            break;
        }
        case ClockConfig::ENABLE :
        {
            PO_DEBUG("clock=%d On", chan);
            break;
        }
        default : ASSERT(0);
    }
    // Other config : TODO

    // Select MultiSynth as the clock source
    data[1] |= 0x0c;

    return i2c->write(addr, data, sizeof(data));
}

bool Si5351::set_multisynth(uint8_t chan, MultisynthConfig *config)
{
    PO_DEBUG("a=%d b=%d c=%d op_div=%d", (int) config->a, (int) config->b, (int) config->c, (int) config->op_divider);

    struct P3 p;
    abc_to_p3(& p, config->a, config->b, config->c);

    return write_msx(chan, & p, config->op_divider);
}

}   //  namespace panglos

//  FIN
