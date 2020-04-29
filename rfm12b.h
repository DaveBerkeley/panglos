
#if !defined(__RFM12B_H__)
#define __RFM12B_H__

    /*
     *  Note :- this driver is still under development.
     *
     *  Do Not Use
     */

#include "spi.h"
#include "gpio.h"
#include "event.h"
#include "msg_queue.h"

namespace panglos {

class RFM12B
{
    SpiDevice *spi;
    GPIO *irq;
    Semaphore *semaphore;

public:
    // Command Message
    class RadioMsg
    {
    public:
        enum Cmd {
            INTERRUPT,
            RX_ENABLE,
            TX_ENABLE,
            IDLE_ENABLE,
        };

        Cmd cmd;
        Semaphore *semaphore;
    };

    enum State {
        S_RX,
        S_TX,
        S_SLEEP,
    };

    State state;

private:
    // Message Queue
    typedef MsgQueue<RadioMsg*> Queue;
    Mutex *mutex;
    Queue::Deque deque;
    Queue *queue;

    typedef uint8_t command[2];

    void send(const command *cmds, int count);
    void send(const command *cmds);

public:

    enum Frequency {
        BAND_315MHZ = 0,
        BAND_433MHZ = 1,
        BAND_868MHZ = 2,
        BAND_915MHZ = 3,
    };

    RFM12B(SpiDevice *spi, GPIO *irq);
    ~RFM12B();

    bool on_power_up(timer_t timeout);
    bool init(Frequency band, uint8_t power);
    void reset();

    // called on GPIO irq
    void _on_gpio_irq();

    // internal functions
    uint16_t _read_status();
    void _rx_enable();
    void _tx_enable();
    void _idle_enable();
    void _on_interrupt();
    uint8_t _data_read();
    void _data_write(uint8_t data);

    void run();

    void _send_msg(RadioMsg *msg, Semaphore *s);
    void kill();
    void rx_enable(Semaphore *s);
    void tx_enable(Semaphore *s);
    void idle_enable(Semaphore *s);
};

}   //  namespace panglos

#endif // __RFM12B_H__

// FIN
