
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>

#include "debug.h"
#include "select.h"
#include "list.h"

#include "esp8266.h"
#include "esp8266_cmd.h"

namespace panglos {

    /*
     *
     */

ESP8266::ESP8266(Output *_uart, UART::Buffer *b, Semaphore *_rd_sem, GPIO *_reset)
:   uart(_uart), rb(b),
    rd_sem(_rd_sem), wait_sem(0), cmd_sem(0), cancel_sem(0),
    gpio_reset(_reset),
    buff(0), in(0), size(1024), 
    dead(false), is_running(false),
    mutex(0), hook(0), commands(Command::next_fn), delete_queue(Command::next_fn), command(0), reading(false)
{
    ASSERT(uart);
    ASSERT(rb);

    cmd_sem = Semaphore::create();
    wait_sem = Semaphore::create();
    cancel_sem = Semaphore::create();
    mutex = Mutex::create();
    buff = (uint8_t*) malloc(size);
}

ESP8266::~ESP8266()
{
    delete cancel_sem;
    delete wait_sem;
    delete cmd_sem;
    delete mutex;
    free(buff);
}

void ESP8266::request_command(Command *cmd)
{
    ASSERT(cmd);
    PO_DEBUG("cmd=%p %s at='%s'", cmd, cmd ? cmd->name : "??", cmd->at);

    commands.append(cmd, mutex);
    cmd_sem->post();
}

void ESP8266::delete_command(Command *cmd)
{
    PO_DEBUG("cmd=%p", cmd);
    delete_queue.push(cmd, mutex);
    cmd->active = false;
    cmd_sem->post();
    cancel_sem->post();
}


void ESP8266::run_command()
{
    ASSERT(command == 0);
    Command *cmd = commands.pop(mutex);

    if (!cmd)
    {
        // command may have been cancelled?
        return;
    }

    if (cmd->cancelled)
    {
        delete_command(cmd);
        return;
    }

    PO_DEBUG("cmd=%p name=%s at='%s'", cmd, cmd->name , cmd->at? cmd->at : "(null)");

    // set the current command
    set_command(cmd, __FUNCTION__);

    if (hook)
    {
        hook->on_command(cmd);
    }

    cmd->start();

    if (cmd->at)
    {
        send_at(cmd->at);
    }
}

    /*
     *
     */

bool ESP8266::start()
{
    AtCommand cmd(this, "AT\r\n", "AT");

    cmd.run();
    if (cmd.result != Command::OK)
    {
        PO_ERROR("failed to start");
        return false;
    }
    return true;
}

    /*
     *
     */

int ESP8266::socket_open(const char *ip, int port, Radio::Transport transport)
{
    Connect cmd(this, ip, port, transport);
    cmd.run();
    if (cmd.result != Command::OK)
    {
        PO_ERROR("failed to place in Client mode");
        return 0;
    }

    return cmd.connected ? 1 : 0;
}

    /*
     *
     */

int ESP8266::socket_close(int fileno)
{
    IGNORE(fileno);
    ASSERT(0); // not yet implemented
    return -1;
}

    /*
     *
     */

int ESP8266::socket_send(int sock, const uint8_t *d, int size)
{
    PO_DEBUG("sock=%d d=%p s=%d", sock, d, size);

    Send cmd(this, d, size);
    cmd.run();

    return size;
}

    /*
     *
     */

bool ESP8266::connect(const char* ssid, const char *pw)
{
    // Set the unit in WiFi client mode
    AtCommand cwmode(this, "AT+CWMODE=1\r\n", "CWMODE");

    cwmode.run();
    if (cwmode.result != Command::OK)
    {
        PO_ERROR("failed to place in Client mode");
        return false;
    }

    // Set the access point's ssid and password
    ConnectAp connect_ap(this, ssid, pw);

    connect_ap.run();
    if (connect_ap.result != Command::OK)
    {
        PO_ERROR("failed to connect to AP");
        return false;
    }

    if (connect_ap.connected)
    {
        PO_REPORT("wifi connected");
    }
    if (connect_ap.ip)
    {
        PO_REPORT("wifi has ip address");
    }

    return connect_ap.connected && connect_ap.ip;
}

    /*
     *
     */

ESP8266::Command *ESP8266::read(Semaphore *s, uint8_t *buffer, int len, int *count)
{
    Read *read = new Read(this, s, buffer, len, count);
    request_command(read);
    return read;
}

void ESP8266::cancel(Command *cmd)
{
    ASSERT(cmd);
    PO_DEBUG("%s %p", cmd->name, cmd);
    cmd->cancelled = true;
    // if command active, wait for it to be removed
    if (cmd->active)
    {
        cancel_sem->wait();
    }
    cmd_sem->post();
}

    /*
     *
     */

void ESP8266::kill()
{
    dead = true;
    wait_sem->post();
}

    /*
     *
     */

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
    event_queue.wait(s, 120000);
    gpio_reset->set(true);
    event_queue.wait(s, 50000);

    rb->reset();
    in = 0;

    PO_DEBUG("reset");
    delete s;
}

    /*
     *
     */

int ESP8266::write(const uint8_t *data, int size)
{
    PO_DEBUG("%p size=%d", data, size);

    int count = 0;

    for (int i = 0; i < size; i++)
    {
        if (!uart->_putc(*data++))
        {
            return count;
        }
        count += 1;
    }

    return count;
}

void ESP8266::send_at(const char *cmd)
{
    PO_DEBUG("send '%s'", cmd);
    uart->_puts(cmd);
    // TODO : remove me
    if (cmd[0] != 'A')
    {
        ASSERT(0);
    }
}

void ESP8266::set_command(Command *cmd, const char *debug)
{
    PO_DEBUG("old=%p new=%p %s", command, cmd, debug);

    if (command && command->cancelled)
    {
        PO_DEBUG("delete push %p", command);
        delete_queue.push(command, mutex);
        cancel_sem->post();
    }
    if (command)
    {
        command->active = false;
    }

    command = cmd;
    if (cmd == 0)
    {
        // fetch the next command ..
        cmd_sem->post();
    }
}

void ESP8266::end_command(Command *cmd, Semaphore *s)
{
    if (commands.remove(cmd, mutex))
    {
        // if pending, removed from queue
        PO_DEBUG("remove pending cmd %s %p", cmd ? cmd->name : "??", cmd);
    }

    if (command == cmd)
    {
        // if active, stop handling it
        set_command(0, __FUNCTION__);
    }

    // Note :- once the command is posted, it is invalid
    // as the Command may go out of scope in the caller.
    if (s)
    {
        s->post();
    }
}

void ESP8266::check(Command *cmd)
{
    // called when ~Command() is called.
    // sanity check that it is not active

    if (cmd == command)
    {
        PO_ERROR("cmd=%p", cmd);
        ASSERT(0);
    }
    if (commands.has(cmd, mutex))
    {
        PO_ERROR("cmd=%p", cmd);
        ASSERT(0);
    }
}

    /*
     *
     */

void ESP8266::process(const uint8_t *text)
{
    ASSERT(text);

    if (!strlen((const char*) text))
    {
        return;
    }

    if (command)
    {
        PO_DEBUG("cmd=%s %p at='%s'", command->name, command, text);
        command->process(text);
    }
}

void ESP8266::process(uint8_t data)
{
    if (command)
    {
        if (command->consume(data))
        {
            // command wants to consume this char
            //PO_DEBUG("consume %#02x", data);
            return;
        }
    }

    if (reading)
    {
        buffers.add(data);
        reading = !buffers.full();
        //PO_DEBUG("reading %#02x %c reading=%d", data, isprint(data) ? data : '.', reading);
        return;
    }

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

    //PO_DEBUG("%02x %c", data, isprint(data) ? data : '.');
    buff[in] = data;
    in = next;

    // Check for the "+IPD," read data string
    if (!strncmp("+IPD,", (const char*) buff, 5))
    {
        if (data == ':')
        {
            buff[next] = '\0';
            create_rx_buffer(buff);
            in = 0;
            return;
        }
    }
}

    /*
     *
     */

void ESP8266::create_rx_buffer(const uint8_t *ipd)
{
    // should have "+IPD,<length>:"
    //PO_DEBUG("got %s", ipd);

    // read the length field.
    int size = 0;
    for (const char *s = (const char *) & ipd[5]; *s != ':'; s++)
    {
        size *= 10;
        if (!isdigit(*s))
        {
            PO_ERROR("bad length '%s'", ipd);
            return;
        }
        size += *s - '0';
    }

    // rationality check
    if ((size < 0) || (size > 1024))
    {
        PO_ERROR("bad buffer size=%d", size);
        return;
    }

    PO_DEBUG("size=%d", size);

    if (!buffers.full())
    {
        // all previous buffers shouls be full.
        // dropped serial data could cause errors here.
        ASSERT(0);
    }

    // Allocate read buff
    buffers.add_buffer(size);
    // redirect reads until done
    reading = true;
}

int ESP8266::read(uint8_t *data, int size)
{
    return buffers.read(data, size);
}

    /*
     *
     */

void ESP8266::run()
{
    PO_DEBUG("");

    Select select(4); // select.wait() adds an event semaphore
    select.add(rd_sem);
    select.add(wait_sem);
    select.add(cmd_sem);

    reset();

    is_running = true;

    while (!dead)
    {
        select.wait(& event_queue, timer_now() + 120000);

        // delete any old messages
        while (!delete_queue.empty())
        {
            Command *c = delete_queue.pop(mutex);
            delete c;
        }

        if (command && command->cancelled)
        {
            delete_command(command);
        }

        if (command == 0)
        {
            run_command();
        }

        // read the rx data ..
        uint8_t buff[128];
        const int n = rb->get(buff, sizeof(buff)-1);
        ASSERT((n >= 0) && (n <= (int)sizeof(buff)));

        for (int i = 0; i < n; i++)
        {
            //PO_DEBUG("%d %#02x", i, buff[i]);
            process(buff[i]);
        }
    }

    while (!delete_queue.empty())
    {
        Command *c = delete_queue.pop(mutex);
        delete c;
    }
}

}   //  namespace panglos

//  FIN
