
#include <gtest/gtest.h>

#include <panglos/debug.h>
#include <panglos/list.h>
#include <panglos/mutex.h>

#include <panglos/vcd.h>

#include <panglos/drivers/i2c_bitbang.h>
#include <panglos/drivers/gpio.h>

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
    SimGpio test;
    
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
    bool sda_response;
    int bit;
    bool stopped;

    VcdWriter *vcd;
    bool verbose;

    I2CSim(VcdWriter *_vcd=0, bool _verbose=false)
    :   sequence(next_seq),
        scl_state(true),
        sda_state(true),
        sda_response(true),
        bit(0),
        stopped(true),
        vcd(_vcd),
        verbose(_verbose)
    {
        sda.set_handlers(set_sda, get_sda, this);
        scl.set_handlers(set_scl, get_scl, this);
        if (vcd)
        {
            vcd->add("sda", true, 1);
            vcd->add("scl", true, 1);
            vcd->add("test", true, 1);
        }
    }

    ~I2CSim()
    {
        // Should have removed all the states
        EXPECT_FALSE(sequence.head);
    }

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
        if (verbose) PO_DEBUG("");
        pop(stopped ? START : RESTART);
        if (verbose) PO_DEBUG("%s", stopped ? "START" : "RESTART");
        // TODO : compare to new item->state? 
        bit = -1;
        stopped = false;
        sda_response = true;
        log_sda();
        test_pulse();
    }

    void next_in_seq(enum State s)
    {
        // next byte xfer?
        if (verbose) PO_DEBUG("pop %s", lut(state_lut, s));
        pop(s);
        bit = 0;
        if (verbose) PO_DEBUG("bit:=%d", bit);
        struct Seq *item = sequence.head;
        if (verbose) PO_DEBUG("state=%s", lut(state_lut, item ? item->state : NONE));
    }

    void stop()
    {
        if (verbose) PO_DEBUG("STOP");
        pop(STOP);
        stopped = true;
        sda_response = true;
        log_sda();
    }

    void log_sda()
    {
        if (!vcd) return;
        const bool s = sda_state & sda_response;
        if (!s)
        {
            vcd->set("sda", false);
            return;
        }
        if (sda_state == 0)
        {
            vcd->set("sda", false);
            return;
        }
        vcd->set("sda", true);
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
        log_sda();
    }

    void test_pulse()
    {
        if (!vcd)   return;
        if (verbose) PO_DEBUG("");
        vcd->set("test", false); 
        wait(); 
        vcd->set("test", true);
        wait(); 
    }

    void set_scl(bool s)
    {
        if (s == scl_state) return;
        if (verbose) PO_DEBUG("s=%d", s);
        if (vcd) vcd->set("scl", s);

        struct Seq *item = sequence.head;

        if (s)
        {
            // rising edge of clock
        }
        else
        {
            // falling edge of clock
            if (!item)
            {
                sda_response = true;
            }
            else
            {
                if (verbose) PO_DEBUG("state=%s bit=%d data=%#x sda_state=%d, sda_response=%d", 
                        lut(state_lut, item->state), 
                        bit, item->data, sda_state, sda_response);

                // peripheral only holds sda low during ACK
                // or when sending data
                sda_response = true;

                if (bit == 7)
                {
                    // ACK period
                    EXPECT_TRUE((item->state == RX) || (item->state == TX));
                    PO_DEBUG("set %s", item->ack ? "ACK" : "NAK");
                    sda_response = !item->ack;

                    get_sda(); // for VCD
                }

                if (item && (item->state == TX))
                {
                    // need to send data to controller
                    PO_DEBUG("!!!!!!! bit=%d  !!!!", bit);
                }

            }
            log_sda();

            if ((!item) || (item->state != STOP))
            {
                bit += 1;
            }
            if (verbose) PO_DEBUG("bit=%d", bit);
            if (bit == 9)
            {
                if (item)
                {
                    next_in_seq(item->state);
                }
            }
        }

        scl_state = s;
    }

    bool get_sda()
    {
        const bool b = sda_state & sda_response;

        struct Seq *item = sequence.head;
        if (verbose) PO_DEBUG("state=%s bit=%d d=%d data=%#x sda_state=%d sda_response=%d", 
                        lut(state_lut, item ? item->state : NONE), 
                        bit, b, item ? item->data : -1, sda_state, sda_response);

        return b;
    }
    bool get_scl()
    {
        ASSERT(0);
        return false;
    }
    void wait()
    {
        if (verbose) PO_DEBUG("");
        if (vcd) vcd->tick();
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

bool verbose = false;

#define VCD_PATH "/tmp/i2c.vcd"
#define SR_PATH  "/tmp/i2c.sr"

TEST(I2C, Start)
{
    VcdWriter vcd(VCD_PATH, SR_PATH);
    I2CSim sim(& vcd, verbose);
    BitBang_I2C i2c(0, & sim.scl, & sim.sda, I2CSim::wait, & sim, verbose);

    vcd.write_header();

    sim.add(I2CSim::START);

    i2c.start();
    EXPECT_TRUE(sim.sequence.empty());
}

TEST(I2C, Stop)
{
    VcdWriter vcd(VCD_PATH, SR_PATH);
    I2CSim sim(& vcd, verbose);
    BitBang_I2C i2c(0, & sim.scl, & sim.sda, I2CSim::wait, & sim, verbose);

    vcd.write_header();

    sim.set_initial(false, false);
    sim.add(I2CSim::STOP);

    i2c.stop();
    EXPECT_TRUE(sim.sequence.empty());
}

TEST(I2C, Probe)
{
    VcdWriter vcd(VCD_PATH, SR_PATH);
    I2CSim sim(& vcd, verbose);
    BitBang_I2C i2c(0, & sim.scl, & sim.sda, I2CSim::wait, & sim, verbose);

    vcd.write_header();

    //  ACK
    sim.add(I2CSim::START);
    sim.add(I2CSim::RX, 0x41, true);
    sim.add(I2CSim::STOP);

    bool ok = i2c.probe(0x20, 1);
    EXPECT_TRUE(ok);
    EXPECT_TRUE(sim.sequence.empty());

    // NAK
    sim.add(I2CSim::START);
    sim.add(I2CSim::RX, 0x41, false);
    sim.add(I2CSim::STOP);

    ok = i2c.probe(0x20, 1);
    EXPECT_FALSE(ok);
    EXPECT_TRUE(sim.sequence.empty());
}

TEST(I2C, ByteWrite)
{
    VcdWriter vcd(VCD_PATH, SR_PATH);
    I2CSim sim(& vcd, verbose);
    BitBang_I2C i2c(0, & sim.scl, & sim.sda, I2CSim::wait, & sim, verbose);

    vcd.write_header();

    // ACK
    sim.add(I2CSim::START);
    sim.add(I2CSim::RX, 0x40, true);
    sim.add(I2CSim::STOP);

    bool ack = false;

    i2c.start();
    uint8_t rd = i2c.io(0x40, & ack);
    i2c.stop();

    EXPECT_EQ(ack, true);
    EXPECT_EQ(rd, 0x40);

    // NAK
    sim.add(I2CSim::START);
    sim.add(I2CSim::RX, 0x66, false);
    sim.add(I2CSim::STOP);

    i2c.start();
    rd = i2c.io(0x66, & ack);
    i2c.stop();

    EXPECT_EQ(ack, false);
    EXPECT_EQ(rd, 0x66);
}

TEST(I2C, BytesWrite)
{
    VcdWriter vcd(VCD_PATH, SR_PATH);
    I2CSim sim(& vcd, verbose);
    BitBang_I2C i2c(0, & sim.scl, & sim.sda, I2CSim::wait, & sim, verbose);

    vcd.write_header();

    // ACK
    sim.add(I2CSim::START);
    sim.add(I2CSim::RX, 0x40, true);
    sim.add(I2CSim::RX, 0xaa, true);
    sim.add(I2CSim::RX, 0x00, true);
    sim.add(I2CSim::RX, 0xff, true);
    sim.add(I2CSim::RX, 0x55, true);
    sim.add(I2CSim::STOP);

    const uint8_t wr[] = { 0xaa, 0x00, 0xff, 0x55 };
    int n = i2c.write(0x20, wr, sizeof(wr));
    EXPECT_EQ(n, 4);
}

#if 0
TEST(I2C, WriteRead)
{
    VcdWriter vcd(VCD_PATH, SR_PATH);
    I2CSim sim(& vcd, verbose);
    BitBang_I2C i2c(0, & sim.scl, & sim.sda, I2CSim::wait, & sim, verbose);

    vcd.write_header();

    // ACK
    sim.add(I2CSim::START);
    sim.add(I2CSim::RX, 0x40, true); // select device write
    sim.add(I2CSim::RX, 0x12, true); // select device write
    sim.add(I2CSim::RESTART);
    sim.add(I2CSim::RX, 0x41, true); // 
    sim.add(I2CSim::TX, 0xab, true);
    sim.add(I2CSim::STOP);

    const uint8_t wr[] = { 0x12, };
    uint8_t rd[] = { 0 };
    int n = i2c.write_read(0x20, wr, sizeof(wr), rd, sizeof(rd));
    EXPECT_EQ(n, sizeof(rd));
    EXPECT_EQ(rd[0],  0xab);
}
#endif

//  FIN
