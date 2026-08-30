
#include <math.h>

#include "panglos/debug.h"

#include "panglos/mutex.h"
#include "panglos/time.h"
#include "panglos/drivers/i2c.h"
#include "panglos/drivers/si5351.h"

namespace panglos {

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

    if (verbose) PO_DEBUG("p1=0x%x p2=0x%x p3=0x%x", (int) p->p1, (int) p->p2, (int) p->p3);
    if (verbose) PO_DEBUG("reg=%d %02x %02x %02x %02x %02x %02x %02x %02x",
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

    if (verbose) PO_DEBUG("p1=0x%x p2=0x%x p3=0x%x", (int) p->p1, (int) p->p2, (int) p->p3);
    if (verbose) PO_DEBUG("reg=%d %02x %02x %02x %02x %02x %02x %02x %02x",
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

Si5351::Si5351(I2C *_i2c, uint8_t _addr, bool _verbose)
:   i2c(_i2c),
    addr(_addr ? _addr : ADDR),
    verbose(_verbose),
    xtal_freq(0)
{
    PO_DEBUG("");
    ASSERT(i2c);
}

    /*
     *
     */

bool Si5351::init(uint32_t xtal_hz, uint32_t ms)
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

        //Time::msleep(5);
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

    //  disable all the output channels
    for (uint8_t chan = 0; chan < 8; chan++)
    {
        ClockConfig config = { .state = ClockConfig::OFF };
        set_clock(chan, & config);
    }
    
    PO_DEBUG("xtal_freq=%d", (int) xtal_hz);
    xtal_freq = xtal_hz;

    return true;
}

void Si5351::set_xtal(XtalConfig *)
{
    // TODO : xtal capacitance etc.
}

bool Si5351::reset_pll(bool a, bool b)
{
    uint8_t mask = 0;
    if (a) mask |= 0x20;
    if (b) mask |= 0x80;

    if (verbose) PO_DEBUG("a=%d b=%d", a, b);
    uint8_t wr[2] { PLL_Reset, mask };
    return i2c->write(addr, wr, sizeof(wr));
}

bool Si5351::set_pll(uint8_t pll, PllConfig *config)
{
    ASSERT((pll == 0) || (pll == 1));

    if (verbose) PO_DEBUG("pll=%d a=%d b=%d c=%d", 
            (int) pll,
            (int) config->a, (int) config->b, (int) config->c);

    struct P3 p;
    abc_to_p3(& p, config->a, config->b, config->c);
    if (!write_pll(pll, & p))
        return false;
 
    // PLL_Input_Source Reg is shared between both PLLs, so need to do a read-modify-write

    uint8_t reg = uint8_t(PLL_Input_Source);
    uint8_t rd;

    if (!i2c->write_read(addr, & reg, sizeof(reg), & rd, sizeof(rd)))
        return false;

    uint8_t state = rd;
    uint8_t mask = (pll == 0) ? 0x04 : 0x08;

    switch (config->source)
    {
        case PllConfig::XTAL : { state &= ~mask; break; }
        case PllConfig::EXT  : { state |= mask; break; }
        case PllConfig::VCXO : break;
        default : ASSERT(00);
    }

    // TODO clock input div for ext clock

    uint8_t data[] = { reg, state };

    if (state == rd) return true; // nothing to change

    if (verbose) PO_DEBUG("reg=%d data=%02x", (int) data[0], (int) data[1]);

    return i2c->write(addr, data, sizeof(data));
}

    /*
     *
     */

bool Si5351::set_clock(uint8_t chan, ClockConfig *config)
{
    ASSERT(chan < 8);

    uint8_t data[2] = { uint8_t(CLK_Control + chan), };
    const char *state = "";

    switch (config->state)
    {
        case ClockConfig::TRI :
        case ClockConfig::HI :
        case ClockConfig::LO :   ASSERT(0); break;
        case ClockConfig::OFF :
        {
            state = "Off";
            data[1] |= 0x80; 
            break;
        }
        case ClockConfig::ENABLE :
        {
            state = "On";
            break;
        }
        default : ASSERT(0);
    }

    switch (config->source)
    {
        case ClockConfig::PLLA : break;
        case ClockConfig::PLLB : data[1] |= 0x20; break;
        default : ASSERT(0);
    }

    if (config->integer_mode)
    {
        data[1] |= 0x40;
    }
    if (config->invert)
    {
        data[1] |= 0x10;
    }

    // Other config : TODO

    data[1] |= 0x0c; // Select MultiSynth as the source for CLK

    if (verbose) PO_DEBUG("reg=%d data=%02x %s", (int) data[0], (int) data[1], state);

    return i2c->write(addr, data, sizeof(data));
}

    /*
     *
     */

bool Si5351::set_multisynth(uint8_t chan, MultisynthConfig *config)
{
    if (verbose) PO_DEBUG("chan=%d a=%d b=%d c=%d op_div=%d", 
            (int) chan,
            (int) config->a, (int) config->b, (int) config->c,
            (int) config->op_divider);

    struct P3 p;
    abc_to_p3(& p, config->a, config->b, config->c);

    return write_msx(chan, & p, config->op_divider);
}

    /*
     *
     */

bool Si5351::set_config(Si5351::Config *config)
{
    if (!set_pll(config->pll, & config->vco))
        return false;
    if (!set_multisynth(config->chan, & config->ms))
        return false;
    if (!reset_pll(config->pll == 0, config->pll == 1))
        return false;
    if (!set_clock(config->chan, & config->clock))
        return false;
    return true;
}

}   //  namespace panglos

//  FIN
