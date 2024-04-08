
#include <stdint.h>

#if defined(ESP32)

extern "C" {
    #include <freertos/FreeRTOS.h>
    #include <freertos/queue.h>
}

#include "panglos/debug.h"

#include "panglos/esp32/hal.h"
#include "panglos/esp32/gpio.h"
#include "panglos/esp32/uart.h"

#include "driver/uart.h"
#include "driver/uart_select.h"

    /*
     *  https://docs.espressif.com/projects/esp-idf/en/latest/esp32c3/api-reference/peripherals/uart.html
     */

using namespace panglos;

    /*
     *
     */

static const LUT uart_event_lut[] = {
    { "UART_DATA", UART_DATA, },
    { "UART_BREAK", UART_BREAK, },
    { "UART_BUFFER_FULL", UART_BUFFER_FULL, },
    { "UART_FIFO_OVF", UART_FIFO_OVF, },
    { "UART_FRAME_ERR", UART_FRAME_ERR, },
    { "UART_PARITY_ERR", UART_PARITY_ERR, },
    { "UART_DATA_BREAK", UART_DATA_BREAK, },
    { "UART_PATTERN_DET", UART_PATTERN_DET, },
    { 0, 0 },
};

static uart_port_t get_port(int p)
{
    // Check the port range for vailidity
    ASSERT(p < UART_NUM_MAX);
    ASSERT(p >= 0);
    return (uart_port_t) p;
}

    /*
     *
     */

class XUART : public ESP_UART
{
    const uart_port_t port;
    uint32_t pin_rx;
    uint32_t pin_tx;
    void (*handler)(In::Event *ev, void *arg);
    void *arg;
    bool verbose;

    virtual int tx(const char* data, int n) override;
    virtual int rx(char* data, int n) override;
    virtual void set_event_handler(void (*fn)(In::Event *ev, void *arg), void *arg) override;
    virtual void tx_flush() override;

public:
    XUART(int id, uint32_t rx, uint32_t tx, uint32_t baud, bool verbose);
    virtual ~XUART();

    void on_cb(uart_select_notif_t notif, int* wake);
    static XUART *xuarts[UART_NUM_MAX];
};

XUART *XUART::xuarts[UART_NUM_MAX];

    /*
     *
     */

static void cb(uart_port_t port, uart_select_notif_t notif, int* wake)
{
    // WARNING : called in UART interrupt
    XUART *uart = XUART::xuarts[port];
    ASSERT(uart);
    uart->on_cb(notif, wake);
}

void XUART::on_cb(uart_select_notif_t notif, int* wake)
{
    if (!handler)
    {
        return;
    }

    UART::Event event = { .code = ERROR, };

    switch (notif)
    {
        case UART_SELECT_READ_NOTIF :   event.code = READ; break;
        case UART_SELECT_WRITE_NOTIF :  event.code = WRITE; break;
        case UART_SELECT_ERROR_NOTIF :  event.code = ERROR; break;
        default : ASSERT(0);
    }

    handler((In::Event*) & event, arg); 
}

XUART::XUART(int id, uint32_t _rx, uint32_t _tx, uint32_t baud, bool _verbose)
:   port(get_port(id)),
    pin_rx(_rx),
    pin_tx(_tx),
    handler(0),
    arg(0),
    verbose(_verbose)
{
    if (pin_rx != -1) ESP_GPIO::mark_used(pin_rx);
    if (pin_tx != -1) ESP_GPIO::mark_used(pin_tx);

    if (verbose) PO_DEBUG("baud=%d rx=%d tx=%d", (int) baud, (int) pin_rx, (int) pin_tx);

    ASSERT(!xuarts[port]);
    xuarts[port] = this;

    uart_config_t config = {
        .baud_rate = (int) baud,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_CTS_RTS,
        .rx_flow_ctrl_thresh = 122,
        .source_clk = UART_SCLK_APB,
    };
    esp_err_t err = uart_param_config(port, & config);
    esp_check(err, __LINE__);
    err = uart_set_pin(port, pin_tx, pin_rx, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    esp_check(err, __LINE__);

    const int buffer_size = 1024 * 2;
    err = uart_driver_install(port, buffer_size, buffer_size, 0, 0, 0);
    esp_check(err, __LINE__);
    
    uart_set_select_notif_callback(port, cb);
}

    /*
     *
     */
XUART::~XUART()
{
    // TODO : uninstall driver etc.
    ESP_GPIO::mark_unused(pin_rx);
    ESP_GPIO::mark_unused(pin_tx);
}

    /*
     * 
     */

int XUART::tx(const char* data, int n)
{
    if (verbose) PO_DEBUG("'%.*s'", n, data);
    return uart_write_bytes(port, data, n);
}

    /*
     * 
     */

int XUART::rx(char* data, int n)
{
    size_t size = 0;
    esp_err_t err = uart_get_buffered_data_len(port, & size);
    esp_check(err, __LINE__);
    if (n > size)
    {
        n = size;
    }
    return uart_read_bytes(port, data, n, 0);
}

    /*
     *
     */

void XUART::tx_flush()
{
    while (true)
    {
        esp_err_t err = uart_wait_tx_done(port, 5);
        esp_check(err, __LINE__);
    }
}

    /*
     * 
     */

void XUART::set_event_handler(void (*fn)(In::Event *ev, void *arg), void *_arg)
{
    handler = fn;
    arg = _arg;
}

    /*
     * 
     */

ESP_UART *ESP_UART::create(int id, uint32_t rx, uint32_t tx, uint32_t baud, bool verbose)
{
    return new XUART(id, rx, tx, baud, verbose);
}

#endif  //  ESP32

//  FIN
