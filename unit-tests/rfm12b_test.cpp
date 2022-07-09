
#include <gtest/gtest.h>

#include <panglos/debug.h>
#include <panglos/thread.h>
#include <panglos/rfm12b.h>

#include "mock.h"

using namespace panglos;

    /*
     *
     */

TEST(RFM12B, RadioBand)
{
    MockPin cs(1), irq(2);
    MockSpi spi;
    SpiDevice dev(& spi, & cs, 0);
    RFM12B radio(& dev, & irq);

    spi.reset();
    radio.init(RFM12B::BAND_315MHZ, 0);
    // band is encoded in the initial command
    EXPECT_EQ(0x80, spi.buff[0]);
    EXPECT_EQ(0xc7, spi.buff[1]);
    EXPECT_TRUE(spi.in > 1);

    spi.reset();
    radio.init(RFM12B::BAND_433MHZ, 0);
    // band is encoded in the initial command
    EXPECT_EQ(0x80, spi.buff[0]);
    EXPECT_EQ(0xd7, spi.buff[1]);
    EXPECT_TRUE(spi.in > 1);

    spi.reset();
    radio.init(RFM12B::BAND_868MHZ, 0);
    // band is encoded in the initial command
    EXPECT_EQ(0x80, spi.buff[0]);
    EXPECT_EQ(0xe7, spi.buff[1]);
    EXPECT_TRUE(spi.in > 1);

    spi.reset();
    radio.init(RFM12B::BAND_915MHZ, 0);
    // band is encoded in the initial command
    EXPECT_EQ(0x80, spi.buff[0]);
    EXPECT_EQ(0xf7, spi.buff[1]);
    EXPECT_TRUE(spi.in > 1);
}

    /*
     *
     */

TEST(RFM12B, TxPower)
{
    MockPin cs(1), irq(2);
    MockSpi spi;
    SpiDevice dev(& spi, & cs, 0);
    RFM12B radio(& dev, & irq);

    // bytes into init sequence
    const int offset = 17;

    spi.reset();
    radio.init(RFM12B::BAND_868MHZ, 0);
    EXPECT_EQ(0x50, spi.buff[offset]);
    EXPECT_TRUE(spi.in > offset);

    spi.reset();
    radio.init(RFM12B::BAND_868MHZ, 7);
    EXPECT_EQ(0x57, spi.buff[offset]);
    EXPECT_TRUE(spi.in > offset);

    spi.reset();
    radio.init(RFM12B::BAND_868MHZ, 3);
    EXPECT_EQ(0x53, spi.buff[offset]);
    EXPECT_TRUE(spi.in > offset);
}

    /*
     *
     */

#if 0
TEST(RFM12B, PowerUp)
{
    mock_setup(true);
    MockPin cs(1), irq(2);
    MockSpi spi;
    SpiDevice dev(& spi, & cs, 0);
    RFM12B radio(& dev, & irq);

    // should time out with irq low
    irq.set(false);
    bool okay = radio.on_power_up((panglos::timer_t) 10000);
    EXPECT_FALSE(okay);

    // check power up sequence
    EXPECT_EQ(0x00, spi.buff[0]);
    EXPECT_EQ(0x00, spi.buff[1]);
    EXPECT_EQ(0x82, spi.buff[2]);
    EXPECT_EQ(0x05, spi.buff[3]);
    EXPECT_EQ(0xb8, spi.buff[4]);
    EXPECT_EQ(0x00, spi.buff[5]);
    // further zeros ...
    EXPECT_EQ(0x00, spi.buff[6]);
    EXPECT_EQ(0x00, spi.buff[7]);
    EXPECT_TRUE(spi.in > 7);

    spi.reset();

    // should pass with irq low
    irq.set(true);
    okay = radio.on_power_up((panglos::timer_t) 10000);
    EXPECT_TRUE(okay);

    // check power up sequence
    EXPECT_EQ(0x00, spi.buff[0]);
    EXPECT_EQ(0x00, spi.buff[1]);
    EXPECT_EQ(0x82, spi.buff[2]);
    EXPECT_EQ(0x05, spi.buff[3]);
    EXPECT_EQ(0xb8, spi.buff[4]);
    EXPECT_EQ(0x00, spi.buff[5]);
    // should not generate any more commands than this
    EXPECT_TRUE(spi.in == 6);

    mock_teardown();
}
#endif

    /*
     *
     */

#if 0
static void * radio_thread(void *arg)
{
    ASSERT(arg);
    RFM12B *radio = (RFM12B*) arg;

    radio->run();

    return 0;
}

TEST(RFM12B, Test)
{
    MockPin cs(1), irq(2);
    MockSpi spi;
    SpiDevice dev(& spi, & cs, 0);
    RFM12B radio(& dev, & irq);

    const bool okay = radio.on_power_up(10000);
    ASSERT(okay);

    const uint8_t power = 3;
    radio.init(RFM12B::BAND_868MHZ, power);

    spi.reset();

    pthread_t thread;
    int err = pthread_create(& thread, 0, radio_thread, & radio);
    EXPECT_EQ(0, err);

    radio._on_gpio_irq();
    radio.rx_enable(0);

    Semaphore *s = Semaphore::create();
    radio.tx_enable(s);
    s->wait();

    radio.kill();

    err = pthread_join(thread, 0);
    EXPECT_EQ(0, err);

    delete s;
}
#endif

//  FIN
