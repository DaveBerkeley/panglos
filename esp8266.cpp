
#include <stdlib.h>
#include <string.h>

#include "debug.h"
#include "select.h"

#include "esp8266.h"

#include "secret.h"

namespace panglos {

ESP8266::ESP8266(Output *_uart, RingBuffer *b, Semaphore *_rd_sem, GPIO *_reset)
:   uart(_uart), rb(b), rd_sem(_rd_sem), wait_sem(0), 
    gpio_reset(_reset), cmd_sem(0), buff(0), in(0), size(1024), dead(false)
{
    ASSERT(uart);
    ASSERT(rb);

    cmd_sem = Semaphore::create();
    wait_sem = Semaphore::create();
    buff = (uint8_t*) malloc(size);
}

ESP8266::~ESP8266()
{
    delete wait_sem;
    delete cmd_sem;
    free(buff);
}

void ESP8266::kill()
{
    dead = true;
    wait_sem->post();
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
    in = 0;

    PO_DEBUG("reset");
    delete s;
}

void ESP8266::send_at(const char *cmd)
{
    PO_DEBUG("send AT%s", cmd);
    uart->_puts("AT");
    uart->_puts(cmd);
    uart->_puts("\r\n");
}

void ESP8266::process(const uint8_t *cmd)
{
    ASSERT(cmd);
    if (!strlen((const char*) cmd))
    {
        return;
    }

    // TODO
    PO_DEBUG("'%s'", cmd);
}

void ESP8266::process(uint8_t data)
{
    int next = in;
    next += 1;
    if (next >= size)
    {
        // Buffer overrun
        PO_ERROR("buffer overrun");
        in = 0;
        return;
    }

    if ((data == '\r') || (data == '\n'))
    {
        // command received
        buff[in] = '\0';
        process(buff);
        in = 0;
        return;
    }

    buff[in] = data;
    in = next;
}

    /*
     *
     */

void ESP8266::run()
{
    Select select;

    select.add(rd_sem);
    select.add(wait_sem);

    reset();

    timer_t last_tx = timer_now();

    send_at("+CWMODE=1"); // station mode
    const char *cmd = "+CWJAP_DEF=\"" SSID "\",\"" PASSWORD "\"";
    send_at(cmd); // connect to wifi

    while (!dead)
    {
        Semaphore *s = select.wait(& event_queue, 120000);
        const bool ready = (s == rd_sem);

        if (!ready)
        {
            timer_t now = timer_now();

            if ((now - last_tx) > (120000 * 10))
            {
                send_at("");
                last_tx = now;
            }

            continue;
        }

        // read the data ..
        uint8_t buff[16];
        const int n = rb->_gets(buff, sizeof(buff)-1);
        ASSERT((n >= 0) && (n <= (int)sizeof(buff)));

        for (int i = 0; i < n; i++)
        {
            process(buff[i]);
        }
    }
}

}   //  namespace panglos

//  FIN
