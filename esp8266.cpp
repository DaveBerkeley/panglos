
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "debug.h"
#include "select.h"
#include "list.h"

#include "esp8266.h"

#include "secret.h"

namespace panglos {

static pList* next_fn(pList item)
{
    ESP8266::Command *cmd = ( ESP8266::Command *) item;
    return (pList*) & cmd->next;
}

    /*
     *
     */
 
ESP8266::ESP8266(Output *_uart, RingBuffer<uint8_t> *b, Semaphore *_rd_sem, GPIO *_reset)
:   uart(_uart), rb(b), rd_sem(_rd_sem), wait_sem(0), 
    gpio_reset(_reset), cmd_sem(0), 
    buff(0), in(0), size(1024), 
    dead(false), is_running(false),
    mutex(0), hook(0), commands(0), command(0)
{
    ASSERT(uart);
    ASSERT(rb);

    cmd_sem = Semaphore::create();
    wait_sem = Semaphore::create();
    mutex = Mutex::create();
    buff = (uint8_t*) malloc(size);
}

ESP8266::~ESP8266()
{
    delete wait_sem;
    delete cmd_sem;
    delete mutex;
    free(buff);
}

void ESP8266::set_hook(Hook *h)
{
    hook = h;
}

void ESP8266::push_command(Command *cmd)
{
    PO_DEBUG("");
    ASSERT(cmd);
    ASSERT(cmd->next == 0);
    ASSERT(cmd->cmd);
    ASSERT(cmd->done);

    list_append((pList *) & commands, (pList) cmd, next_fn, mutex);
    cmd_sem->post();
}

void ESP8266::run_command()
{
    PO_DEBUG("");
    Command *cmd = (Command*) list_pop((pList *) & commands, next_fn, mutex);
    ASSERT(cmd);
    ASSERT(cmd->done);

    if (hook)
    {
        hook->on_command(cmd);
    }

    send_at(cmd->cmd);

    // set the current command
    command = cmd;
}

bool ESP8266::connect(const char* ssid, const char *pw)
{
    // Set the unit in WiFi client mode
    Command cmd;
    cmd.cmd = "AT+CWMODE=1\r\n";
    cmd.done = Semaphore::create();
    cmd.next = 0;

    push_command(& cmd);
    cmd.done->wait();
    if (cmd.result != Command::OK)
    {
        return false;
    }

    // Set the access point's ssid and password
    char buff[64];
    snprintf(buff, sizeof(buff), "AT+CWJAP_DEF=\"%s\",\"%s\"\r\n", ssid, pw);
    cmd.cmd = buff;

    push_command(& cmd);
    cmd.done->wait();
    delete cmd.done;
    return cmd.result == Command::OK;
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
        // TODO : try software reset of device?
        // See "AT+RST\r\n"
        return;
    }

    PO_DEBUG("");
    Semaphore *s = Semaphore::create();

    gpio_reset->set(false);
    event_queue.wait(s, 50000);
    gpio_reset->set(true);
    event_queue.wait(s, 10000);

    rb->reset();
    in = 0;

    PO_DEBUG("reset");
    delete s;
}

void ESP8266::send_at(const char *cmd)
{
    PO_DEBUG("send AT%s", cmd);
    uart->_puts(cmd);
}

void ESP8266::process(const uint8_t *text)
{
    ASSERT(text);

    if (!strlen((const char*) text))
    {
        return;
    }

    PO_DEBUG("'%s'", text);

    if (!command)
    {
        // no command currently executing
        return;
    }

    // Check for completion of the command
    if (!strcmp((const char*) text, "OK"))
    {
        command->result = Command::OK;
        command->done->post();
        command = 0; // done
        return;
    }

    // cmd needs to hook the rx data and decode it ...
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
    PO_DEBUG("");
    Select select;

    select.add(rd_sem);
    select.add(wait_sem);
    select.add(cmd_sem);

    reset();

    is_running = true;

    timer_t last_tx = timer_now();

    while (!dead)
    {
        PO_DEBUG("");
        Semaphore *s = select.wait(& event_queue, 120000);
        const bool ready = (s == rd_sem);

        if (s == cmd_sem)
        {
            run_command();
            continue;
        }

        if (!ready)
        {
            timer_t now = timer_now();

            if ((now - last_tx) > (120000 * 10))
            {
                //send_at("AT\r\n");
                last_tx = now;
            }

            continue;
        }

        // read the data ..
        uint8_t buff[16];
        const int n = rb->get(buff, sizeof(buff)-1);
        ASSERT((n >= 0) && (n <= (int)sizeof(buff)));

        for (int i = 0; i < n; i++)
        {
            process(buff[i]);
        }
    }
}

}   //  namespace panglos

//  FIN
