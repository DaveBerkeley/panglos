
#include <stdlib.h>
#include <string.h>

#include "debug.h"

#include "esp8266.h"

namespace panglos {

ESP8266::ESP8266(UART *_uart, RingBuffer *b, GPIO *_reset)
: uart(_uart), rb(b), gpio_reset(_reset), semaphore(0), buff(0), in(0), size(1024)
{
    ASSERT(uart);
    ASSERT(rb);

    semaphore = Semaphore::create();
    buff = (uint8_t*) malloc(size);
}

ESP8266::~ESP8266()
{
    delete semaphore;
    delete buff;
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
    uart->send("AT", 2);
    uart->send(cmd, strlen(cmd));
    uart->send("\r\n", 2);
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

    if ((data == '\n') && (in > 0) && (buff[in-1] == '\r'))
    {
        // command received
        buff[in-1] = '\0';
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
    reset();

    send_at("E0"); // echo off
    timer_t last_tx = timer_now();

    while (true)
    {
        const bool ready = rb->wait(& event_queue, 120000);

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
        const int n = rb->gets(buff, sizeof(buff)-1);
        ASSERT((n >= 0) && (n <= (int)sizeof(buff)));

        for (int i = 0; i < n; i++)
        {
            process(buff[i]);
        }
    }
}

}   //  namespace panglos

//  FIN
