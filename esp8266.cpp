
#include <stdio.h> // TODO remove me


#include "debug.h"

#include "esp8266.h"

namespace panglos {

ESP8266::ESP8266(UART *_uart, RingBuffer *b, GPIO *_reset)
: uart(_uart), rb(b), gpio_reset(_reset), semaphore(0)
{
    ASSERT(uart);
    ASSERT(rb);
    semaphore = Semaphore::create();
}

ESP8266::~ESP8266()
{
    delete semaphore;
}

void ESP8266::reset()
{
    if (!gpio_reset)
    {
        return;
    }

    PO_DEBUG("");
    Semaphore *s = Semaphore::create();

    gpio_reset->set(false);
    event_queue.wait(s, 120000);
    gpio_reset->set(true);
    event_queue.wait(s, 50000);

    rb->reset();

    PO_DEBUG("reset");
    delete s;
}

#define HAL_UART_ERROR_NONE              0x00000000U   /*!< No error            */
#define HAL_UART_ERROR_PE                0x00000001U   /*!< Parity error        */
#define HAL_UART_ERROR_NE                0x00000002U   /*!< Noise error         */
#define HAL_UART_ERROR_FE                0x00000004U   /*!< Frame error         */
#define HAL_UART_ERROR_ORE               0x00000008U   /*!< Overrun error       */
#define HAL_UART_ERROR_DMA               0x00000010U   /*!< DMA transfer error  */


static void pr_error(char *buff, int size, uint32_t err)
{
    if (err == HAL_UART_ERROR_NONE)
    {
        buff[0] = '\0';
    }

    if (err & HAL_UART_ERROR_PE)
    {
        const int n = snprintf(buff, size, "PE ");
        buff += n;
        size -= n;
        if (size <= 0)
        {
            return;
        }
    }

    if (err & HAL_UART_ERROR_NE)
    {
        const int n = snprintf(buff, size, "NE ");
        buff += n;
        size -= n;
        if (size <= 0)
        {
            return;
        }
    }

    if (err & HAL_UART_ERROR_FE)
    {
        const int n = snprintf(buff, size, "FE ");
        buff += n;
        size -= n;
        if (size <= 0)
        {
            return;
        }
    }

    if (err & HAL_UART_ERROR_ORE)
    {
        const int n = snprintf(buff, size, "ORE ");
        buff += n;
        size -= n;
        if (size <= 0)
        {
            return;
        }
    }

    if (err & HAL_UART_ERROR_DMA)
    {
        const int n = snprintf(buff, size, "DMA ");
        buff += n;
        size -= n;
        if (size <= 0)
        {
            return;
        }
    }
}


void ESP8266::run()
{
    reset();

    int fail_count = 0;

    while (true)
    {
        const bool ready = rb->wait(& event_queue, 120000);

        if(!ready)
        {
            fail_count += 1;
            if (fail_count >= 10)
            {
                reset();
                fail_count = 0;
                continue;
            }

            char buff[24];
            pr_error(buff, sizeof(buff), uart->get_error());
            PO_DEBUG("%d %s", fail_count, buff);
            uart->send("AT\r\n", 4);
            continue;
        }

        // read the data ..
        uint8_t buff[16];
        const int n = rb->gets(buff, sizeof(buff)-1);
        ASSERT((n >= 0) && (n <= (int)sizeof(buff)));

        if (n)
        {
            buff[n] = '\0';
            PO_DEBUG("%s", buff);
            fail_count = 0;
        }
    }
}

}   //  namespace panglos

//  FIN
