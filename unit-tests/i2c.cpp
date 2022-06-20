
#include <gtest/gtest.h>

#include <panglos/debug.h>
#include <panglos/list.h>
#include <panglos/gpio.h>
#include <panglos/mutex.h>
#include <panglos/i2c_bitbang.h>

#include <panglos/vcd.h>

#include "i2c_utils.h"
#include "mock.h"

using namespace panglos;

    /*
     *
     */


class I2CSim
{
public:
    SimGpio sda;
    SimGpio scl;
    
    enum State {
        NONE,
        START,
        RESTART,
        STOP,

        RX,
        TX,
    };

    static const LUT state_lut[];

    struct Seq
    {
        struct Seq *next;
        enum State state;
        uint8_t data;
        bool ack;
    };

    static struct Seq **next_seq(struct Seq *item) { return & item->next; }

    List<struct Seq *> sequence;

    bool scl_state;
    bool sda_state;
    int bit;

    bool verbose;

    I2CSim(bool _verbose=false)
    :   sequence(next_seq),
        scl_state(true),
        sda_state(true),
        bit(0),
        verbose(_verbose)
    {
        sda.set_handlers(set_sda, get_sda, this);
        scl.set_handlers(set_scl, get_scl, this);
    }

    ~I2CSim() { }

    void set_initial(bool _sda, bool _scl)
    {
        sda_state = _sda;
        scl_state = _scl;
    }

    /*
     *
     */

    void pop(enum State expect)
    {
        struct Seq *item = sequence.pop(0);
        ASSERT(item);
        EXPECT_EQ(item->state, expect);
        delete item;
    }

    /*
     *  Seq transitions
     */

    void start()
    {
        if (verbose) PO_DEBUG("START");
        pop(START);
        bit = -1;
    }

    void next_in_seq()
    {
        // next byte xfer?
        if (verbose) PO_DEBUG("STOP");
        pop(RX);
        bit = 0;
    }

    void stop()
    {
        if (verbose) PO_DEBUG("");
        pop(STOP);
    }

    /*
     *  GPIO callbacks
     */

    void set_sda(bool s)
    {
        if (s == sda_state) return;
        if (verbose) PO_DEBUG("s=%d", s);

        if (scl_state && !s)
        {
            start();
        }
        if (scl_state && s)
        {
            stop();
        }

        sda_state = s;
    }
    void set_scl(bool s)
    {
        if (s == scl_state) return;
        if (verbose) PO_DEBUG("s=%d", s);

        if (s)
        {
            // rising edge of clock, read data
        }
        else
        {
            // falling edge of clock
            bit += 1;
            if (verbose) PO_DEBUG("bit=%d", bit);
        }

        scl_state = s;
    }
    bool get_sda()
    {
        struct Seq *item = sequence.head;
        ASSERT(item);
        ASSERT((bit >= 0));
        const uint8_t mask = (uint8_t) (1 << (7 - bit));

        const bool d = item->data & mask;

        if (verbose) PO_DEBUG("state=%s bit=%d mask=%#x d=%d", lut(state_lut, item->state), bit, mask, d);

        switch (item->state)
        {
            case RX :
            {
                if (mask)
                {
                    EXPECT_EQ(d, sda_state); // data bits
                    return sda_state;
                }
                else
                {
                    EXPECT_TRUE(sda_state); // ack - Controller should set hi
                    const bool ack = !item->ack;
                    next_in_seq();
                    return ack; // ACK
                }
            }
            default : ASSERT(0);
        }

        return false;
    }
    bool get_scl()
    {
        ASSERT(0);
        return false;
    }
    void wait()
    {
        if (verbose) PO_DEBUG("");
    }

        /*
         *
         */

    void add(enum State s, uint8_t n=0, bool ack=false)
    {
        struct Seq *seq = new struct Seq;
        seq->next = 0;
        seq->state = s;
        seq->data = n;
        seq->ack = ack;
        sequence.append(seq, 0);
    }

    static int show_(struct Seq *item, void *arg)
    {
        IGNORE(arg);
        ASSERT(item);
        PO_DEBUG("state=%s d=%#x, ack=%d", lut(state_lut, item->state), item->data, item->ack);
        return 0;
    }

    void show()
    {
        sequence.visit(show_, 0, 0);
    }

    // static functions to map to GPIO callbacks
    static I2CSim* pthis(void *arg)         { ASSERT(arg); return (I2CSim*) arg; }
    static void wait(void *arg)             { pthis(arg)->wait(); }
    static void set_sda(bool s, void *arg)  { pthis(arg)->set_sda(s); }
    static void set_scl(bool s, void *arg)  { pthis(arg)->set_scl(s); }
    static bool get_sda(void *arg)          { return pthis(arg)->get_sda(); }
    static bool get_scl(void *arg)          { return pthis(arg)->get_scl(); }
};


const LUT I2CSim::state_lut[] = {
    {   "NONE", NONE, },
    {   "START", START, },
    {   "RESTART", RESTART, },
    {   "STOP", STOP, },
    {   "RX", RX, },
    {   "TX", TX, },
    { 0, 0 },
};

#if 0

static bool verbose = true;

TEST(I2C, BitHi)
{
    VcdWriter vcd("/tmp/i2c.vcd");

    I2CSim sim(& vcd, verbose);
    BitBang_I2C i2c(0, & sim.scl, & sim.sda, wait, & sim, verbose);

    vcd.write_header();    

    // Set initial state
    sim.scl.set(true);
    sim.sda.set(true);
    sim.reset();

    sim.add_data(0xff, false);

    i2c.start();
    uint8_t rd = i2c.bit_io(1);

    EXPECT_EQ(rd, true);

    I2CSim::State s[] = { 
        I2CSim::START, 
        I2CSim::BIT,
        I2CSim::NONE };
    sim.verify(s);

    vcd.sigrok_write("/tmp/i2c.sr");    
}

TEST(I2C, BitLo)
{
    VcdWriter vcd("/tmp/i2c.vcd");

    I2CSim sim(& vcd, verbose);
    BitBang_I2C i2c(0, & sim.scl, & sim.sda, wait, & sim, verbose);

    vcd.write_header();    

    // Set initial state
    sim.scl.set(true);
    sim.sda.set(true);
    sim.reset();

    sim.add_data(0xff, false);

    i2c.start();
    uint8_t rd = i2c.bit_io(0);

    EXPECT_EQ(rd, false);

    I2CSim::State s[] = { 
        I2CSim::START, 
        I2CSim::BIT,
        I2CSim::NONE };
    sim.verify(s);

    vcd.sigrok_write("/tmp/i2c.sr");    
}

TEST(I2C, BitAck)
{
    VcdWriter vcd("/tmp/i2c.vcd");

    I2CSim sim(& vcd, verbose);
    BitBang_I2C i2c(0, & sim.scl, & sim.sda, wait, & sim, verbose);

    vcd.write_header();    

    // Set initial state
    sim.scl.set(true);
    sim.sda.set(true);
    sim.reset();

    sim.add_data(0x00, false);

    i2c.start();
    uint8_t rd = i2c.bit_io(1);

    EXPECT_EQ(rd, false);

    I2CSim::State s[] = { 
        I2CSim::START, 
        I2CSim::BIT,
        I2CSim::NONE };
    sim.verify(s);

    vcd.sigrok_write("/tmp/i2c.sr");    
}

TEST(I2C, BitNak)
{
    VcdWriter vcd("/tmp/i2c.vcd");

    I2CSim sim(& vcd, verbose);
    BitBang_I2C i2c(0, & sim.scl, & sim.sda, wait, & sim, verbose);

    vcd.write_header();    

    // Set initial state
    sim.scl.set(true);
    sim.sda.set(true);
    sim.reset();

    sim.add_data(0xff, true);

    i2c.start();
    uint8_t rd = i2c.bit_io(1);

    EXPECT_EQ(rd, true);

    I2CSim::State s[] = { 
        I2CSim::START, 
        I2CSim::BIT,
        I2CSim::NONE };
    sim.verify(s);

    vcd.sigrok_write("/tmp/i2c.sr");    
}

TEST(I2C, ByteWrite)
{
    VcdWriter vcd("/tmp/i2c.vcd");

    I2CSim sim(& vcd, verbose);
    BitBang_I2C i2c(0, & sim.scl, & sim.sda, wait, & sim, verbose);

    vcd.write_header();    

    // Set initial state
    sim.scl.set(true);
    sim.sda.set(true);
    sim.reset();

    sim.add_data(0xff, false);
    sim.add_data(0xff, true); // xx

    bool ack = false;

    i2c.start();
    uint8_t rd = i2c.io(0x40, & ack);
    i2c.stop();

    EXPECT_TRUE(ack);
    EXPECT_EQ(rd, 0x40);

    PO_DEBUG("rd=%#x ack=%d", rd, ack);

    I2CSim::State s[] = { 
        I2CSim::START, 
        I2CSim::BIT, I2CSim::BIT, I2CSim::BIT, I2CSim::BIT, 
        I2CSim::BIT, I2CSim::BIT, I2CSim::BIT, I2CSim::BIT, 
        I2CSim::ACK,
        I2CSim::STOP, 
        I2CSim::NONE };
    sim.verify(s);

    vcd.sigrok_write("/tmp/i2c.sr");    
}

TEST(I2C, ByteRead)
{
    VcdWriter vcd("/tmp/i2c.vcd");

    I2CSim sim(& vcd, verbose);
    BitBang_I2C i2c(0, & sim.scl, & sim.sda, wait, & sim, verbose);

    vcd.write_header();    

    // Set initial state
    sim.scl.set(true);
    sim.sda.set(true);
    sim.reset();

    sim.add_data(0xff, false);
    sim.add_data(0xff, true); // xx

    bool ack = false;

    i2c.start();
    uint8_t rd = i2c.io(0x41, & ack);
    i2c.stop();

    EXPECT_TRUE(ack);
    EXPECT_EQ(rd, 0x41);

    PO_DEBUG("rd=%#x ack=%d", rd, ack);

    I2CSim::State s[] = { 
        I2CSim::START, 
        I2CSim::BIT, I2CSim::BIT, I2CSim::BIT, I2CSim::BIT, 
        I2CSim::BIT, I2CSim::BIT, I2CSim::BIT, I2CSim::BIT, 
        I2CSim::ACK,
        I2CSim::STOP, 
        I2CSim::NONE 
    };
    sim.verify(s);

    vcd.sigrok_write("/tmp/i2c.sr");    
}

TEST(I2C, ByteNak)
{
    I2CSim sim;
    BitBang_I2C i2c(0, & sim.scl, & sim.sda, wait, & sim, verbose);

    // Set initial state
    sim.scl.set(true);
    sim.sda.set(true);
    sim.reset();

    sim.add_data(0xff, true);
    sim.add_data(0xff, true); // xx

    bool ack = false;

    i2c.start();
    uint8_t rd = i2c.io(0xaa, & ack);
    i2c.stop();

    EXPECT_FALSE(ack);

    PO_DEBUG("rd=%#x ack=%d", rd, ack);

    I2CSim::State s[] = { 
        I2CSim::START, 
        I2CSim::BIT, I2CSim::BIT, I2CSim::BIT, I2CSim::BIT, 
        I2CSim::BIT, I2CSim::BIT, I2CSim::BIT, I2CSim::BIT, 
        I2CSim::ACK,
        I2CSim::STOP, 
        I2CSim::NONE };
    sim.verify(s);
}

TEST(I2C, WriteRead)
{
    VcdWriter vcd("/tmp/i2c.vcd");

    I2CSim sim(& vcd, verbose);
    BitBang_I2C i2c(0, & sim.scl, & sim.sda, wait, & sim, verbose);

    vcd.write_header();    

    // Set initial state
    sim.scl.set(true);
    sim.sda.set(true);
    sim.reset();

    sim.add_data(0xff, false); // op wr
    sim.add_data(0xff, false); // reg
    sim.add_data(0xff, false); // restart, op rd
    sim.add_data(0xaa, false); // rd value
    sim.add_data(0xff, false); // xx

    uint8_t wr = 0x12;
    uint8_t rd;

    int n = i2c.write_read(0x40, & wr, 1, & rd, 1);

    vcd.sigrok_write("/tmp/i2c.sr");    
    
    EXPECT_EQ(n, 1);

    I2CSim::State s[] = { 
        I2CSim::START, 
        I2CSim::BIT, I2CSim::BIT, I2CSim::BIT, I2CSim::BIT, 
        I2CSim::BIT, I2CSim::BIT, I2CSim::BIT, I2CSim::BIT, 
        I2CSim::ACK,
        I2CSim::STOP, 
        I2CSim::NONE
    };
    sim.verify(s);
}

#endif


bool verbose = true;

TEST(I2C, Start)
{
    I2CSim sim;
    BitBang_I2C i2c(0, & sim.scl, & sim.sda, I2CSim::wait, & sim, verbose);

    sim.add(I2CSim::START);

    i2c.start();
    EXPECT_TRUE(sim.sequence.empty());
}

TEST(I2C, Stop)
{
    I2CSim sim;
    BitBang_I2C i2c(0, & sim.scl, & sim.sda, I2CSim::wait, & sim, verbose);

    sim.set_initial(false, false);
    sim.add(I2CSim::STOP);

    i2c.stop();
    EXPECT_TRUE(sim.sequence.empty());
}

TEST(I2C, Probe)
{
    I2CSim sim;
    BitBang_I2C i2c(0, & sim.scl, & sim.sda, I2CSim::wait, & sim, verbose);

    sim.add(I2CSim::START);
    sim.add(I2CSim::RX, 0x41, true);
    //sim.add(I2CSim::RX, 0x12, true);
    //sim.add(I2CSim::RESTART);
    //sim.add(I2CSim::RX, 0x41, true);
    //sim.add(I2CSim::TX, 0xa5, true);
    sim.add(I2CSim::STOP);

    i2c.probe(0x20, 1);
    EXPECT_TRUE(sim.sequence.empty());
}

//  FIN
