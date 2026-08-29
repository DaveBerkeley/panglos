
#pragma once

namespace panglos {

class I2C;
class Out;

class Si5351
{
    I2C *i2c;
    uint8_t addr;

    uint32_t xtal_freq;

    struct P3
    {
        uint32_t p1;
        uint32_t p2;
        uint32_t p3;
    };

    static void abc_to_p3(struct P3 *p, uint32_t a, uint32_t b, uint32_t c);
    static void set_p123(uint8_t *data, struct P3 *p);
    bool write_pll(uint8_t pll, struct P3 *p);
    bool write_msx(uint8_t chan, struct P3 *p, uint8_t op_div);
    bool loss_of_lock(uint8_t pll);
 
public:
    const static uint8_t ADDR = 0x60; // possibly 0x61

    Si5351(I2C *i2c, uint8_t addr=0);

    bool init(uint32_t timeout_ms=1000);

    // 1..3 clock sources
    // 2 PLLS
    // 0..N Multisynth blocks
    // 0..M outputs

    struct XtalConfig
    {
        uint32_t freq;
        //  load capacitance
    };

    void set_xtal(XtalConfig *config);

    struct PllConfig
    {
        //  PLL A,B :
        //  multisynth a, b, c
        uint32_t a, b, c;
        //  clock source xtal,vcxo,ext (only xtal avail on my boards)
        enum Source { XTAL=0, VCXO, EXT };
        enum Source source;
        //  divider (only for ext. clock)
    };

    bool set_pll(uint8_t pll, PllConfig *config);

    bool reset_pll(bool a, bool b);

    struct MultisynthConfig
    {
        //  Clock 0..7
        //  multisynth a, b, c
        uint32_t a, b, c;
        //  output divider
        uint8_t op_divider;
    };

    bool set_multisynth(uint8_t chan, MultisynthConfig *config);

    struct ClockConfig
    {
        //  Clock 0..7
        enum ClockState { ENABLE, HI, LO, TRI, OFF };
        ClockState state;
        enum Source { PLLA, PLLB };
        Source source;
        bool integer_mode;
        bool invert;
        //  drive current
        //  source direct (bypass divider)
        //  drive current
        //  invert
        //  initial phase
    };

    bool set_clock(uint8_t chan, ClockConfig *config);

};

};  //  namespace panglos

//  FIN
