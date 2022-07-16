
#include <gtest/gtest.h>

#include <panglos/debug.h>
#include <panglos/drivers/mcp23s17.h>
#include <panglos/drivers/keyboard.h>
#include <panglos/drivers/i2c_bitbang.h>

#include <panglos/vcd.h>

#include "i2c_utils.h"
#include "mock.h"

using namespace panglos;

    /*
     *
     */

TEST(MCP23S17, ReadWrite)
{
    MockPin cs(0);
    MockSpi spi;
    SPI_MCP23S17 chip(& spi, & cs, 0);
    spi.reset();

    // check the write does the expected thing
    chip.write(MCP23S17::R_GPIOA, 0xAA);
    EXPECT_EQ(0x40, spi.buff[0]);
    EXPECT_EQ(0x12, spi.buff[1]);
    EXPECT_EQ(0xAA, spi.buff[2]);
    spi.reset();

    uint8_t data;
    spi.set_read(0x55);
    const bool ok = chip.read(MCP23S17::R_GPIOA, & data);
    EXPECT_TRUE(ok);
    EXPECT_EQ(0x41, spi.buff[0]);
    EXPECT_EQ(0x12, spi.buff[1]);
    EXPECT_EQ(0x55, data);
}

    /*
     *
     */

TEST(MCP23S17, Cache)
{
    MockPin cs(0);
    MockSpi spi;
    SPI_MCP23S17 chip(& spi, & cs, 0);
    spi.reset();

    // check the write writes data to cache, not hw
    chip.write_cache(MCP23S17::R_GPIOA, 0xdb);
    EXPECT_EQ(0, spi.in);

    // force a hardware write
    chip.flush_cache(MCP23S17::R_GPIOA);
    EXPECT_EQ(0x40, spi.buff[0]);
    EXPECT_EQ(0x12, spi.buff[1]);
    EXPECT_EQ(0xdb, spi.buff[2]);
    spi.reset();

    // writing the same data, the cache should stay !dirty
    chip.write_cache(MCP23S17::R_GPIOA, 0xdb);
    EXPECT_EQ(0, spi.in);
    // force a hardware write
    chip.flush_cache(MCP23S17::R_GPIOA);
    EXPECT_EQ(0, spi.in);

    // reading from the cache : should not do any hw io
    uint8_t data;
    data = chip.read_cache(MCP23S17::R_GPIOA);
    EXPECT_EQ(0xdb, data);
    EXPECT_EQ(0, spi.in);

    // writing different data, the cache should become dirty
    chip.write_cache(MCP23S17::R_GPIOA, 0xab);
    Cache *cache = chip.get_cache(MCP23S17::R_GPIOA);
    EXPECT_TRUE(cache);
    EXPECT_EQ(true, cache->dirty);
    EXPECT_EQ(0, spi.in);

    // force a hardware write
    chip.flush_cache(MCP23S17::R_GPIOA);
    EXPECT_EQ(0x40, spi.buff[0]);
    EXPECT_EQ(0x12, spi.buff[1]);
    EXPECT_EQ(0xab, spi.buff[2]);
    EXPECT_EQ(false, cache->dirty);
    spi.reset();

    // Try the other port
    // writing data, the cache should become dirty
    chip.write_cache(MCP23S17::R_GPIOB, 0xee);
    cache = chip.get_cache(MCP23S17::R_GPIOB);
    EXPECT_TRUE(cache);
    EXPECT_EQ(true, cache->dirty);
    EXPECT_EQ(0, spi.in);

    // force a hardware write
    chip.flush_cache(MCP23S17::R_GPIOB);
    EXPECT_EQ(0x40, spi.buff[0]);
    EXPECT_EQ(0x13, spi.buff[1]);
    EXPECT_EQ(0xee, spi.buff[2]);
    EXPECT_EQ(false, cache->dirty);
    spi.reset();
}


    /*
     *
     */

TEST(MCP23S17, CacheMask)
{
    MockPin cs(0);
    MockSpi spi;
    SPI_MCP23S17 chip(& spi, & cs, 0);
    spi.reset();

    Cache *cache = chip.get_cache(MCP23S17::R_GPIOA);
    EXPECT_TRUE(cache);

    uint8_t data;

    data = cache->read();
    EXPECT_EQ(0, data);

    cache->set_mask(0x10);
    data = cache->read();
    EXPECT_EQ(0x10, data);

    cache->set_mask(0x04);
    data = cache->read();
    EXPECT_EQ(0x14, data);

    cache->set_mask(0x03);
    data = cache->read();
    EXPECT_EQ(0x17, data);

    cache->clr_mask(0x80);
    data = cache->read();
    EXPECT_EQ(0x17, data);

    cache->clr_mask(0x10);
    data = cache->read();
    EXPECT_EQ(0x07, data);

    cache->clr_mask(0xf2);
    data = cache->read();
    EXPECT_EQ(0x05, data);

    cache->set_mask(0xf2);
    data = cache->read();
    EXPECT_EQ(0xf7, data);
}

    /*
     *
     */

TEST(MCP23S17, GpioAutoFlush)
{
    MockPin cs(0);
    MockSpi spi;
    SPI_MCP23S17 chip(& spi, & cs, 0);
    spi.reset();

    GPIO *pin = chip.make_gpio(MCP23S17::PORTB, 7, MCP23S17::OUTPUT, true);

    // check the init sequence
    // read / modify write of the IODIR reg
    EXPECT_EQ(0x41, spi.buff[0]); // read current state
    EXPECT_EQ(0x01, spi.buff[1]); // io direction reg
    //EXPECT_EQ(0x00, spi.buff[2]); // OUTPUT
    EXPECT_EQ(0x40, spi.buff[3]); // write modified state
    EXPECT_EQ(0x01, spi.buff[4]); // io direction reg
    EXPECT_EQ(0x00, spi.buff[5]); // OUTPUT
    EXPECT_EQ(6, spi.in);
    spi.reset();

    // set bit 7 in the io direction reg
    pin->set(true);
    EXPECT_EQ(0x40, spi.buff[0]); // read current state
    EXPECT_EQ(0x13, spi.buff[1]); // data reg
    EXPECT_EQ(0x80, spi.buff[2]); // data
    EXPECT_EQ(3, spi.in);
    spi.reset();
 
    // set bit 7 in the io direction reg
    // the data is unchanged, so no write takes place
    pin->set(true);
    EXPECT_EQ(0, spi.in);

    pin->set(false);
    EXPECT_EQ(0x40, spi.buff[0]); // read current state
    EXPECT_EQ(0x13, spi.buff[1]); // data reg
    EXPECT_EQ(0x00, spi.buff[2]); // data
    EXPECT_EQ(3, spi.in);
    spi.reset();

    pin->set(true);
    EXPECT_EQ(0x40, spi.buff[0]); // read current state
    EXPECT_EQ(0x13, spi.buff[1]); // data reg
    EXPECT_EQ(0x80, spi.buff[2]); // data
    EXPECT_EQ(3, spi.in);
    spi.reset();

    // make a second pin on the same port
    GPIO *p2 = chip.make_gpio(MCP23S17::PORTB, 2, MCP23S17::OUTPUT, true);
    EXPECT_EQ(0x41, spi.buff[0]); // read current state
    EXPECT_EQ(0x01, spi.buff[1]); // io direction reg
    //EXPECT_EQ(0x00, spi.buff[2]); // OUTPUT
    EXPECT_EQ(0x40, spi.buff[3]); // write modified state
    EXPECT_EQ(0x01, spi.buff[4]); // io direction reg
    EXPECT_EQ(0x00, spi.buff[5]); // OUTPUT
    EXPECT_EQ(6, spi.in);
    spi.reset();
 
    p2->set(true);
    EXPECT_EQ(0x40, spi.buff[0]); // read current state
    EXPECT_EQ(0x13, spi.buff[1]); // data reg
    EXPECT_EQ(0x84, spi.buff[2]); // data
    EXPECT_EQ(3, spi.in);
    spi.reset();

    p2->set(false);
    EXPECT_EQ(0x40, spi.buff[0]); // read current state
    EXPECT_EQ(0x13, spi.buff[1]); // data reg
    EXPECT_EQ(0x80, spi.buff[2]); // data
    EXPECT_EQ(3, spi.in);
    spi.reset();

    pin->set(false);
    EXPECT_EQ(0x40, spi.buff[0]); // read current state
    EXPECT_EQ(0x13, spi.buff[1]); // data reg
    EXPECT_EQ(0x00, spi.buff[2]); // data
    EXPECT_EQ(3, spi.in);
    spi.reset();

    delete pin;
    delete p2;
}

    /*
     *
     */

TEST(MCP23S17, GpioNoFlush)
{
    MockPin cs(0);
    MockSpi spi;
    SPI_MCP23S17 chip(& spi, & cs, 0);
    spi.reset();

    GPIO *pin = chip.make_gpio(MCP23S17::PORTB, 7, MCP23S17::OUTPUT, false);

    // check the init sequence
    // read / modify write of the IODIR reg
    EXPECT_EQ(0x41, spi.buff[0]); // read current state
    EXPECT_EQ(0x01, spi.buff[1]); // io direction reg
    //EXPECT_EQ(0x00, spi.buff[2]); // OUTPUT
    EXPECT_EQ(0x40, spi.buff[3]); // write modified state
    EXPECT_EQ(0x01, spi.buff[4]); // io direction reg
    EXPECT_EQ(0x00, spi.buff[5]); // OUTPUT
    EXPECT_EQ(6, spi.in);
    spi.reset();

    // set bit 7 in the io direction reg
    pin->set(true);
    // data is cached
    EXPECT_EQ(0, spi.in);
    EXPECT_TRUE(pin->get());
    EXPECT_EQ(0, spi.in);

    // check the flushing
    pin->flush();
    EXPECT_EQ(0x40, spi.buff[0]); // read current state
    EXPECT_EQ(0x13, spi.buff[1]); // data reg
    EXPECT_EQ(0x80, spi.buff[2]); // data
    EXPECT_EQ(3, spi.in);
    spi.reset();
 
    // make a second pin on the same port
    GPIO *p2 = chip.make_gpio(MCP23S17::PORTB, 2, MCP23S17::OUTPUT, false);
    EXPECT_EQ(0x41, spi.buff[0]); // read current state
    EXPECT_EQ(0x01, spi.buff[1]); // io direction reg
    //EXPECT_EQ(0x00, spi.buff[2]); // OUTPUT
    EXPECT_EQ(0x40, spi.buff[3]); // write modified state
    EXPECT_EQ(0x01, spi.buff[4]); // io direction reg
    EXPECT_EQ(0x00, spi.buff[5]); // OUTPUT
    EXPECT_EQ(6, spi.in);
    spi.reset();
 
    p2->set(true);
    // data is cached
    EXPECT_EQ(0, spi.in);
    EXPECT_TRUE(p2->get());
    EXPECT_EQ(0, spi.in);

    pin->set(true);
    // data is cached
    EXPECT_EQ(0, spi.in);
    EXPECT_TRUE(pin->get());
    EXPECT_EQ(0, spi.in);

    // get the cache for this gpio port
    Cache *cache = chip.get_cache(MCP23S17::R_GPIOB);
    EXPECT_TRUE(cache);
    // both bits set
    EXPECT_EQ(0x84, cache->read());

    // flush p2 data
    p2->flush();
    EXPECT_EQ(0x40, spi.buff[0]); // read current state
    EXPECT_EQ(0x13, spi.buff[1]); // data reg
    EXPECT_EQ(0x84, spi.buff[2]); // data
    EXPECT_EQ(3, spi.in);
    spi.reset();

    // flush pin data
    pin->flush();
    // port already flushed by the other pin
    EXPECT_EQ(0, spi.in);

    delete pin;
    delete p2;
}

    /*
     *
     */

TEST(MCP23S17, GpioInput)
{
    MockPin cs(0);
    MockSpi spi;
    SPI_MCP23S17 chip(& spi, & cs, 0);
    spi.reset();

    GPIO *pin = chip.make_gpio(MCP23S17::PORTB, 7, MCP23S17::OUTPUT, true);

    // check the init sequence
    // read / modify write of the IODIR reg
    EXPECT_EQ(0x41, spi.buff[0]); // read current state
    EXPECT_EQ(0x01, spi.buff[1]); // io direction reg
    //EXPECT_EQ(0x00, spi.buff[2]); // OUTPUT
    EXPECT_EQ(0x40, spi.buff[3]); // write modified state
    EXPECT_EQ(0x01, spi.buff[4]); // io direction reg
    EXPECT_EQ(0x00, spi.buff[5]); // OUTPUT
    EXPECT_EQ(6, spi.in);
    spi.reset();

    GPIO *p2 = chip.make_gpio(MCP23S17::PORTB, 4, MCP23S17::INPUT_PU, true);
 
    // check the init sequence
    // read / modify write of the IODIR reg
    EXPECT_EQ(0x41, spi.buff[0]); // read current state
    EXPECT_EQ(0x01, spi.buff[1]); // io direction reg
    //EXPECT_EQ(0x00, spi.buff[2]); // 
    EXPECT_EQ(0x40, spi.buff[3]); // write modified state
    EXPECT_EQ(0x01, spi.buff[4]); // io direction reg
    EXPECT_EQ(0x10, spi.buff[5]); // INPUT

    // read / modify write the pull-up register
    EXPECT_EQ(0x41, spi.buff[6]); // read current state
    EXPECT_EQ(0x0d, spi.buff[7]); // pullup reg
    //EXPECT_EQ(0x00, spi.buff[8]); // 
    EXPECT_EQ(0x40, spi.buff[9]); // write modified state
    EXPECT_EQ(0x0d, spi.buff[10]); // pullup reg
    EXPECT_EQ(0x10, spi.buff[11]); // data
    EXPECT_EQ(12, spi.in);
    spi.reset();

    uint8_t data;

    data = p2->get();
    EXPECT_EQ(0, data);
    EXPECT_EQ(0x41, spi.buff[0]); // read
    EXPECT_EQ(0x13, spi.buff[1]); // gpio reg
    EXPECT_EQ(0x00, spi.buff[2]); // data
    EXPECT_EQ(3, spi.in);
    spi.reset();

    spi.set_read(0x10);
    data = p2->get();
    EXPECT_EQ(1, data);
    spi.reset();

    delete pin;
    delete p2;
}

    /*
     *
     */

TEST(MCP23S17, SpiAddr)
{
    MockPin cs(0);
    MockSpi spi;
    // use a SPI address of 7 (can be 0..7)
    SPI_MCP23S17 chip(& spi, & cs, 7);
    spi.reset();

    GPIO *pin = chip.make_gpio(MCP23S17::PORTB, 7, MCP23S17::OUTPUT, true);

    // check the init sequence
    // read / modify write of the IODIR reg
    EXPECT_EQ(0x4f, spi.buff[0]); // read current state
    EXPECT_EQ(0x01, spi.buff[1]); // io direction reg
    //EXPECT_EQ(0x00, spi.buff[2]); // OUTPUT
    EXPECT_EQ(0x4e, spi.buff[3]); // write modified state
    EXPECT_EQ(0x01, spi.buff[4]); // io direction reg
    EXPECT_EQ(0x00, spi.buff[5]); // OUTPUT
    EXPECT_EQ(6, spi.in);
    spi.reset();

    delete pin;
}

    /*
     *
     */

TEST(MCP23S17, SpiAddrEnable)
{
    MockPin cs(0);

    {
        MockSpi spi;
        SPI_MCP23S17 chip(& spi, & cs, 3);
 
        // check the HAEN bit gets set in ICON
        EXPECT_EQ(0x40, spi.buff[0]); // write 
        EXPECT_EQ(0x0a, spi.buff[1]); // ICON
        EXPECT_EQ(0x48, spi.buff[2]); // enable HEAN | MIRROR
        EXPECT_EQ(3, spi.in);
    }

    // Check the work around for the silicon bug
    // MCP23S17 Rev. A  Silicon Errata
    {
        MockSpi spi;
        SPI_MCP23S17 chip(& spi, & cs, 7);
 
        // check the HAEN bit gets set in ICON
        EXPECT_EQ(0x48, spi.buff[0]); // write 
        EXPECT_EQ(0x0a, spi.buff[1]); // ICON
        EXPECT_EQ(0x48, spi.buff[2]); // enable HEAN | MIRROR
        EXPECT_EQ(3, spi.in);
    }
}

    /*
     *
     */

static void on_irq(void *arg)
{
    ASSERT(arg);
    int *i = (int*) arg;
    *i += 1;
}

TEST(MCP23S17, GpioIrq)
{
    MockPin cs(0);
    MockSpi spi;
    SPI_MCP23S17 chip(& spi, & cs, 0);
    spi.reset();

    const int bit = 4;
    GPIO *pin = chip.make_gpio(MCP23S17::PORTB, bit, MCP23S17::IT_FALLING, true);
 
    // check the init sequence
    // IODIRx |= mask
    // read / modify write of the IODIR reg
    EXPECT_EQ(0x41, spi.buff[0]); // read current state
    EXPECT_EQ(0x01, spi.buff[1]); // io direction reg
    //EXPECT_EQ(0x00, spi.buff[2]); // 
    EXPECT_EQ(0x40, spi.buff[3]); // write modified state
    EXPECT_EQ(0x01, spi.buff[4]); // io direction reg
    EXPECT_EQ(0x10, spi.buff[5]); // INPUT

    // IODIRx |= mask
    // read / modify write the pull-up register
    EXPECT_EQ(0x41, spi.buff[6]); // read current state
    EXPECT_EQ(0x0d, spi.buff[7]); // pullup reg
    //EXPECT_EQ(0x00, spi.buff[8]); // 
    EXPECT_EQ(0x40, spi.buff[9]); // write modified state
    EXPECT_EQ(0x0d, spi.buff[10]); // pullup reg
    EXPECT_EQ(0x10, spi.buff[11]); // data
 
    // DEFVALx &= ~mask
    // read / modify write the defval register
    EXPECT_EQ(0x41, spi.buff[12]); // read current state
    EXPECT_EQ(0x07, spi.buff[13]); // defval reg
    //EXPECT_EQ(0x00, spi.buff[14]); // 
    EXPECT_EQ(0x40, spi.buff[15]); // write modified state
    EXPECT_EQ(0x07, spi.buff[16]); // defval reg
    EXPECT_EQ(0x00, spi.buff[17]); // clear data

    // GPINTENx |= mask
    // read / modify write the gpinten register
    EXPECT_EQ(0x41, spi.buff[18]); // read current state
    EXPECT_EQ(0x05, spi.buff[19]); // gpinten reg
    //EXPECT_EQ(0x00, spi.buff[20]); // 
    EXPECT_EQ(0x40, spi.buff[21]); // write modified state
    EXPECT_EQ(0x05, spi.buff[22]); // gpinten reg
    EXPECT_EQ(0x10, spi.buff[23]); // set data

    EXPECT_EQ(24, spi.in);
 
    spi.reset();

    int count = 0;
    pin->set_interrupt_handler(GPIO::CHANGE, on_irq, & count);

    const uint8_t zero[2] = { 0, 0 };
    spi.set_read(zero, sizeof(zero));
    // this would get called on the ARM GPIO interrupt
    chip.get_interrupt_handler()->execute();
    // Check the SPI reads the INTFx regs
    // read / modify write of the IODIR reg
    EXPECT_EQ(0x41, spi.buff[0]); // read current state
    EXPECT_EQ(0x0e, spi.buff[1]); // INTFA
    //EXPECT_EQ(0x00, spi.buff[2]); // INTFA
    //EXPECT_EQ(0x00, spi.buff[3]); // INTFB
    //EXPECT_EQ(0xxx, spi.buff[4]); // don't care
    //EXPECT_EQ(0xxx, spi.buff[5]); // don't care
    //EXPECT_EQ(0xxx, spi.buff[6]); // GPIOA
    //EXPECT_EQ(0xxx, spi.buff[7]); // GPIOB
    EXPECT_EQ(8, spi.in);
    // the INTFx regs return 0x00, 0x00, so no irq response
    EXPECT_EQ(0, count);

    // PORTB, bit 4
    spi.reset();
    const uint8_t pb4[2] = { 0, 0x10 };
    spi.set_read(pb4, sizeof(pb4));
    
    chip.get_interrupt_handler()->execute();
    // Check the SPI reads the INTFx regs
    // read / modify write of the IODIR reg
    EXPECT_EQ(0x41, spi.buff[0]); // read current state
    EXPECT_EQ(0x0e, spi.buff[1]); // INTFA
    //EXPECT_EQ(0x00, spi.buff[2]); // INTFA
    //EXPECT_EQ(0x10, spi.buff[3]); // INTFB
    //EXPECT_EQ(0xxx, spi.buff[4]); // don't care
    //EXPECT_EQ(0xxx, spi.buff[5]); // don't care
    //EXPECT_EQ(0xxx, spi.buff[6]); // GPIOA
    //EXPECT_EQ(0xxx, spi.buff[7]); // GPIOB
    EXPECT_EQ(8, spi.in);
    // the INTFx regs return 0x00, 0x10, so irq response
    EXPECT_EQ(1, count);

    spi.reset();
    delete pin;

    // Check that the GPINTENx bit is cleared
    EXPECT_EQ(0x41, spi.buff[0]); // read
    EXPECT_EQ(0x05, spi.buff[1]); // GPINTENB
    //EXPECT_EQ(0x00, spi.buff[2]); // 
    EXPECT_EQ(0x40, spi.buff[3]); // write
    EXPECT_EQ(0x05, spi.buff[4]); // GPINTENB
    EXPECT_EQ(0x00, spi.buff[5]); // clear the bit
    EXPECT_EQ(6, spi.in);
}

    /*
     *
     */

class I2CSlaveSim
{
private:

    enum State {
        IDLE,
        START,
        BIT,
        ACK,
        STOP,
    };

    static const LUT state_lut[];

    State state;
    bool sda_state;
    bool scl_state;
    int bit;
    bool first;
    int data;
    bool verbose;

    void set_state(State s)
    {
        if (state == s) return;
        state = s;
        if (verbose) PO_DEBUG("%s", lut(state_lut, state));
        if (vcd) 
        {
            vcd->set("s0", state & 0x01);
            vcd->set("s1", state & 0x02);
            vcd->set("s2", state & 0x04);
        }
    }

public:
    SimGpio sda;
    SimGpio scl;
    VcdWriter *vcd;

    I2CSlaveSim(VcdWriter *_vcd=0, bool _verbose=false)
    :   state(IDLE),
        sda_state(true),
        scl_state(true),
        bit(0),
        first(false),
        data(0),
        verbose(_verbose),
        vcd(_vcd)
    {
        sda.set_handlers(set_sda, get_sda, this);
        scl.set_handlers(set_scl, get_scl, this);

        if (vcd)
        {
            vcd->add("scl", true, 1);
            vcd->add("sda", true, 1);
            vcd->add("s0", false, 1);
            vcd->add("s1", false, 1);
            vcd->add("s2", false, 1);
        }
    }

private:

    void new_byte()
    {
        bit = 0;
        data = 0;
    }

    void start()
    {
        set_state(START);
        first = true;
        new_byte();
    }

    void stop()
    {
        set_state(STOP);
    }

    void on_sda(bool s)
    {
        if (s == sda_state) return;
        if (verbose) PO_DEBUG("s=%d", s);
        if (scl_state && sda_state && !s)
        {
            // TODO : validate this is a correct transition
            start();
        }
        if (scl_state && s && !sda_state)
        {
            // TODO : validate this is a correct transition
            stop();
        }
        sda_state = s;
        if (vcd) vcd->set("sda", get_sda());
    }
    void on_scl(bool s)
    {
        if (vcd) vcd->set("scl", s);
        if (s == scl_state) return;
        if (verbose) PO_DEBUG("s=%d", s);
        switch (state)
        {
            case IDLE : break;
            case START :
            {
                if (!s)
                {
                    // First clock low
                    set_state(BIT);
                    new_byte();
                }
                break;
            }
            case BIT :
            {
                if (s)
                {
                    // Clock in data on rising edge
                    data <<= 1;
                    data += sda_state ? 1 : 0;
                    if (verbose) PO_DEBUG("bit=%d rx=%d data=%#x", bit, sda_state, data);
                    bit += 1;
                }
                if ((bit == 8) && !s)
                {
                    // last bit : ACK/NAK
                    if (verbose) PO_DEBUG("rx=%#x", data);
                    set_state(ACK);
                }
                break;
            }
            case ACK :
            {
                if (!s)
                {
                    set_state(BIT);
                    new_byte();
                }
                break;
            }
            case STOP :
            {
                if (verbose) PO_DEBUG("stop");
                break;
            }
            default :
            {
                if (verbose) PO_ERROR("state=%d", state);
                ASSERT(0);
            }
        }
        scl_state = s;
        if (vcd) vcd->set("sda", get_sda());
    }
    bool get_sda()
    {
        const bool s = (state == ACK) ? 0 : 1;
        // open-drain bus, so the AND of the two signals
        const bool r = sda_state && s;
        if (verbose) PO_DEBUG("get=%d", r);
        return r;
    }
    bool get_scl()
    {
        return 1;
    }

    // Callbacks for GPIOs
    static I2CSlaveSim* pthis(void *arg) { ASSERT(arg); return (I2CSlaveSim *) arg; }

    static void set_sda(bool s, void *arg)  { pthis(arg)->on_sda(s); }
    static void set_scl(bool s, void *arg)  { pthis(arg)->on_scl(s); }
    static bool get_sda(void *arg)          { return pthis(arg)->get_sda(); }
    static bool get_scl(void *arg)          { return pthis(arg)->get_scl(); }
};

const LUT I2CSlaveSim::state_lut[] = {
    {   "IDLE", IDLE, },
    {   "START", START, },
    {   "BIT", BIT, },
    {   "ACK", ACK, },
    {   "STOP", STOP, },
    {   0, 0, },
};


    /*
     *
     */

void on_key(void *arg)
{
    IGNORE(arg);
    Keyboard *kb = (Keyboard*) arg;
    const uint8_t keys = kb->read_keys();
    PO_DEBUG("keys=%#x", keys);
}

TEST(MCP23S17, Keyboard)
{
    VcdWriter vcd("/tmp/x.vcd");

    I2CSlaveSim slave(& vcd, true);
    BitBang_I2C i2c(0, & slave.scl, & slave.sda, 0, 0, true);

    vcd.write_header();

    i2c.probe(0x20, 1);

#if 1
    I2C_MCP23S17 chip(& i2c, 0x20);
    MockPin irq(2, MockPin::CHANGE);
    irq.set(1);

    Keyboard keyboard(& chip, & irq);

    keyboard.set_key_event(on_key, & keyboard);

    keyboard.init();

    PO_DEBUG("irq=0");
    irq.set(0);
    PO_DEBUG("irq=1");
    irq.set(1);
#endif
    vcd.sigrok_write("/tmp/x.sr");
}

//  FIN
