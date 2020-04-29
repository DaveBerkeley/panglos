
#include <stdint.h>

#include "debug.h"

#include "timer.h"
#include "event.h"

#include "rfm12b.h"

    /*
     *  Note :- this driver is still under development.
     *
     *  Do Not Use
     */

// RFM12B command codes
#define RF_RECEIVER_ON  { 0x82, 0xDD }
#define RF_XMITTER_ON   { 0x82, 0x3D }
#define RF_IDLE_MODE    { 0x82, 0x0D }
#define RF_SLEEP_MODE   { 0x82, 0x05 }
#define RF_WAKEUP_MODE  { 0x82, 0x07 }
#define RF_TXREG_WRITE  { 0xB8, 0x00 }
#define RF_RX_FIFO_READ { 0xB0, 0x00 }
#define RF_WAKEUP_TIMER { 0xE0, 0x00 }
#define RF_READ_STATUS  { 0x00, 0x00 }

uint16_t calc_crc(const uint8_t *bu, uint16_t strt, uint16_t end)
{
    const uint16_t poly = 0xA001;
    uint16_t crc = 0xFFFF;

    for (uint16_t i = strt; i <= end; i++)
    {
        const uint8_t b = bu[i];
        crc ^= b;

        for (int n = 1; n <= 8; n++)
        {
            if (crc & 0x0001)
            {
                crc = (crc>>1) ^ poly;
            }
            else
            {
                crc = crc >> 1;
            }
        }
    }

    return crc;
}

    /*
     *
     */

namespace panglos {

static void irq_handler(void *arg)
{
    ASSERT(arg);
    RFM12B* radio = (RFM12B*) arg;
    radio->_on_gpio_irq();
}

RFM12B::RFM12B(SpiDevice *_spi, GPIO *_irq)
: spi(_spi), irq(_irq), semaphore(0), state(S_SLEEP), mutex(0), queue(0)
{
    PO_DEBUG("WARNING : DEVELOPMENT INCOMPLETE");
    ASSERT(spi);
    ASSERT(irq);

    semaphore = Semaphore::create();
    mutex = Mutex::create_critical_section();
    queue = new Queue(& deque, mutex, semaphore);

    irq->set_interrupt_handler(irq_handler, this);
}

RFM12B::~RFM12B()
{
    irq->set_interrupt_handler(0, 0);

    delete queue;
    delete mutex;

    delete semaphore;
}

	/*
	 *
	 */

void RFM12B::send(const command *cmd)
{
    spi->write((const uint8_t *) cmd, sizeof(*cmd));
    //const uint8_t *s = (const uint8_t*) cmd;
    //PO_DEBUG("%#02x%02x", s[0], s[1]);
}

void RFM12B::send(const command *cmds, int count)
{
    for (int i = 0; i < count; i++)
    {
        send(& cmds[i]);
    }
}

	/*
	 *
	 */

bool RFM12B::on_power_up(timer_t timeout)
{
    // Technique taken from JeeLabs library at
    // https://github.com/LowPowerLab
    const command cmd[] = {
        RF_READ_STATUS,
        RF_SLEEP_MODE,
        //RF_TXREG_WRITE,
    };
    const int size = sizeof(cmd) / sizeof(command);
    send(cmd, size);

    const timer_t start = timer_now();
    timer_t now = start;

    // Wait for the irq line to go high ...

    while (!irq->get())
    {
    	now = timer_now();
    	if ((now - start) > timeout)
    	{
            PO_ERROR("Timeout waiting for initialisation");
            return false;
    	}

        _read_status();

    	event_queue.wait(semaphore, 1000);
    }

    PO_DEBUG("Radio Ready, took %d", now - start);
    return true;
}

bool RFM12B::init(Frequency freq_band, uint8_t tx_power)
{
    const uint8_t tx_rate = 0x08;
    ASSERT(tx_power < 8);

    const command init_state[] = {
        { 0x80, uint8_t(0xc7 | ((freq_band & 0x03) << 4)), }, // EL (ena TX), EF (ena RX FIFO), 12.0pF
        { 0xA6, 0x40 },                     // Frequency is exactly 434/868/915MHz
        { 0xC6, 0x00 + tx_rate },           // Air transmission baud rate: 0x08= ~38.31Kbps
        { 0x94, 0xA2 },                     // VDI,FAST,134kHz,0dBm,-91dBm
        { 0xC2, 0xAC },                     // AL,!ml,DIG,~DQD4
#if 0 // net_id
        { 0xCA, 0x83 },                     // FIFO8,2-SYNC,!ff,DR
        { 0xCE, 0x00 | net_id },            // SYNC=2DXX
#else
        { 0xCA, 0x8B },                     // FIFO8,1-SYNC,!ff,DR
        { 0xCE, 0x2D },                     // SYNC=2D
#endif
        { 0xC4, 0x83 },                     // @PWR,NO RSTRIC,!st,!fi,OE,EN
        { 0x98, uint8_t(0x50 | tx_power) }, // !mp,90kHz,MAX OUT //last byte=power level: 0=highest, 7=lowest
        { 0xCC, 0x77 },                     // OB1,OB0, LPX,!ddy,DDIT,BW0
        { 0xE0, 0x00 },                     // wake up timer command : not used
        { 0xC8, 0x00 },                     // low duty cycle command : not used
        { 0xC0, 0x43 },                     // Clock output (1.66MHz), Low Voltage threshold (2.55V)
    };

    const int size = sizeof(init_state) / sizeof(command);
    send(init_state, size);

    return 0;
}

void RFM12B::reset()
{
    const command cmd[] = {
        { 0x80, 0x08, }, // 1 Configuration Setting Command
        { 0x82, 0x08, }, // 2 Power Management Command 
        { 0xA6, 0x80, }, // 3 Frequency Setting Command 
        { 0xC6, 0x23, }, // 4 Data Rate Command 
        { 0x90, 0x80, }, // 5 Receiver Control Command 
        { 0xC2, 0x2C, }, // 6 Data Filter Command 
        { 0xCA, 0x80, }, // 7 FIFO and Reset Mode Command 
        { 0xCE, 0xD4, }, // 8 Synchron Pattern Command 
        { 0xB0, 0x00, }, // 9 Receiver FIFO Read Command 
        { 0xC4, 0xF7, }, // 10 AFC Command 
        { 0x98, 0x00, }, // 11 TX Configuration Control Command 
        { 0xCC, 0x77, }, // 12 PLL Setting Command 
        { 0xB8, 0xAA, }, // 13 Transmitter Register Write Command 
        { 0xE1, 0x96, }, // 14 Wake-Up Timer Command 
        { 0xC8, 0x0E, }, // 15 Low Duty-Cycle Command 
        { 0xC0, 0x00, }, // 16  Low Battery Detector and Microcontroller Clock Divider Command 
        //RF_READ_STATUS, // 17 Status Read Command
    };

    const int size = sizeof(cmd) / sizeof(command);
    send(cmd, size);
}

    /*
     *
     */

/*                    start RX
 *  0xCA81            FIFO init
 *  0xCA83            FIFO activ
 *  0x82D8            RX on
 *
 *                    get byte
 *  0x0000            get status, ask IR
 *  0xB000            get byte
 *
 *                    after RX
 *  0x8258            !RX, !TX
 *  0xCA81            FIFO init
 *
 *
 *                    start TX
 *  0x8278            TX on
 *
 *                    put byte
 *  0x82bb            put byte bb
 *  0x0000            get status ask IR
 *
 *                    after TX
 *  0x8258            !RX, !TX
 *
 */

void RFM12B::_rx_enable()
{
    const command cmd[] = {
        { 0xca, 0x83 }, // enable FIFO
        RF_RECEIVER_ON,
    };

    const int size = sizeof(cmd) / sizeof(command);
    send(cmd, size);
    state = S_RX;
}

void RFM12B::_tx_enable()
{
    const command cmd = RF_XMITTER_ON;
    send(& cmd);
    state = S_TX;
}

void RFM12B::_idle_enable()
{
    const command cmd = RF_IDLE_MODE;
    send(& cmd);
    state = S_SLEEP;
}

    /*
     *
     */

void RFM12B::_send_msg(RadioMsg *msg, Semaphore *s)
{
    ASSERT(msg);
    msg->semaphore = s;
    ASSERT(queue);
    queue->put(msg);
}

void RFM12B::_on_gpio_irq()
{
    static RadioMsg msg = { RadioMsg::INTERRUPT, 0 };
    _send_msg(& msg, 0);
}

void RFM12B::rx_enable(Semaphore *s)
{
    static RadioMsg msg = { RadioMsg::RX_ENABLE, 0 };
    _send_msg(& msg, s);
}

void RFM12B::tx_enable(Semaphore *s)
{
    static RadioMsg msg = { RadioMsg::TX_ENABLE, 0 };
    _send_msg(& msg, s);
}

void RFM12B::idle_enable(Semaphore *s)
{
    static RadioMsg msg = { RadioMsg::IDLE_ENABLE, 0 };
    _send_msg(& msg, s);
}

void RFM12B::kill()
{
    queue->put(0);
}

typedef struct {
    int code;
    const char *text;
}   Label;

static const Label cmds[] = {
    { RFM12B::RadioMsg::INTERRUPT,   "INTERRUPT" },
    { RFM12B::RadioMsg::RX_ENABLE,   "RX_ENABLE" },
    { RFM12B::RadioMsg::TX_ENABLE,   "TX_ENABLE" },
    { RFM12B::RadioMsg::IDLE_ENABLE, "IDLE_ENABLE" },
    { 0, 0 },
};

static const char *get_code(int code, const Label *labels, const char *def="unknown")
{
    for (const Label *label = labels; label->text; label++)
    {
        if (code == label->code)
        {
            return label->text;
        }
    }
    return def;
}

    /*
     *
     */

enum Status {
    RGIT = (1 << 15), // TX register is ready to receive the next byte (Can be cleared by Transmitter Register Write Command)
    FFIT = (1 << 15), // The number of data bits in the RX FIFO has reached the pre-programmed limit (Can be cleared by any of the FIFO read methods)
    POR = (1 << 14), // Power-on reset (Cleared after Status Read Command)
    RGUR = (1 << 13), // TX register under run, register over write (Cleared after Status Read Command)
    FFOV = (1 << 13), // RX FIFO overflow (Cleared after Status Read Command)
    WKUP = (1 << 12), // Wake-up timer overflow (Cleared after Status Read Command)
    EXT = (1 << 11), // Logic level on interrupt pin (pin 16) changed to low (Cleared after Status Read Command)
    LBD = (1 << 10), // Low battery detect, the power supply voltage is below the pre-programmed limit
    FFEM = (1 << 9), // FIFO is empty
    ATS = (1 << 8), // Antenna tuning circuit detected strong enough RF signal
    RSSI = (1 << 8), // The strength of the incoming signal is above the pre-programmed limit
    DQD = (1 << 7), // Data quality detector output
    CRL = (1 << 6), // Clock recovery locked
    ATGL = (1 << 5), // Toggling in each AFC cycle
    //OFFS // (6)MSB of the measured frequency offset (sign of the offset value)
    //OFFS // (3) -
    //OFFS // (0)Offset value to be added to the value of the frequency control parameter (Four LSB bits)
};

uint16_t RFM12B::_read_status()
{
    const command cmd[] = { RF_READ_STATUS, };
    uint8_t buff[sizeof(cmd)];

    const int size = sizeof(cmd) / sizeof(command);
    spi->read((const uint8_t*) cmd, buff, size);

    const uint16_t status = (buff[0] << 8) + buff[1];

    PO_DEBUG("%#04x %s %s %s %s %s %s %s %s %s %s %s", status,
            (status & RGIT) ? "RGIT/FFIT" : ".",
            (status & POR) ? "POR" : ".",
            (status & RGUR) ? "RGUR/FFOV" : ".",
            (status & WKUP) ? "WKUP" : ".",
            (status & EXT) ? "EXT" : ".",
            (status & LBD) ? "LBD" : ".",
            (status & FFEM) ? "FFEM" : ".",
            (status & ATS) ? "ATS/RSSI" : ".",
            (status & DQD) ? "DQD" : ".",
            (status & CRL) ? "CRL" : ".",
            (status & ATGL) ? "ATGL" : ".");

    return status;
}

uint8_t RFM12B::_data_read()
{
    const command cmd[] = {
        RF_RX_FIFO_READ,
    };
    uint8_t buff[sizeof(cmd)];

    const int size = sizeof(cmd) / sizeof(command);
    spi->read((const uint8_t*) cmd, buff, size);

    PO_DEBUG("%#02x %#02x", buff[0], buff[1]);
    return buff[1];
}

void RFM12B::_data_write(uint8_t data)
{
    const command cmd[] = {
        { 0xB8, data },
    };
    const int size = sizeof(cmd) / sizeof(command);
    send(cmd, size);

    PO_DEBUG("%#02x", data);
}

void RFM12B::_on_interrupt()
{
    uint16_t status = _read_status();

    switch (state)
    {
        case S_RX:
        {
            // DO RX
            ASSERT(status & FFIT);
            _data_read();

            const command cmd[] = {
                { 0xCA, 0x81, }, // disable FIFO to turn off the irq
                { 0xCA, 0x83, }, // enable the FIFO
            };
            const int size = sizeof(cmd) / sizeof(command);
            send(cmd, size);
            break;
        }
        case S_TX:
        {
            // DO RX
            break;
        }
        case S_SLEEP:
        {
            break;
        }
        default : ASSERT(0);
    }
}

    /*
     *
     */

void RFM12B::run()
{
    ASSERT(queue);

    PO_DEBUG("");

    rx_enable(0);

    while (true)
    {
        RadioMsg *msg = queue->wait();

        if (!msg)
        {
            PO_DEBUG("exit");
            return;
        }

        const char *s= get_code(msg->cmd, cmds);
        PO_DEBUG("%p %d %s", msg, msg->cmd, s);

        // execute the command
        switch (msg->cmd)
        {
            case RadioMsg::RX_ENABLE : _rx_enable(); break;
            case RadioMsg::TX_ENABLE : _tx_enable(); break;
            case RadioMsg::INTERRUPT : _on_interrupt(); break;
            case RadioMsg::IDLE_ENABLE : _idle_enable(); break;
            default : ASSERT(0);
        }

        // signal that the command has been completed
        if (msg->semaphore)
        {
            msg->semaphore->post();
        }
    }
}

}   //  namespace panglos

//  FIN
