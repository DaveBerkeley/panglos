
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "debug.h"
#include "select.h"
#include "list.h"

#include "esp8266.h"

namespace panglos {

static ESP8266::Command **next_fn(ESP8266::Command *cmd)
{
    return & cmd->next;
}

    /*
     *
     */
 
ESP8266::ESP8266(Output *_uart, UART::Buffer *b, Semaphore *_rd_sem, GPIO *_reset)
:   uart(_uart), rb(b), rd_sem(_rd_sem), wait_sem(0), 
    gpio_reset(_reset), cmd_sem(0), 
    buff(0), in(0), size(1024), 
    dead(false), is_running(false),
    mutex(0), hook(0), commands(next_fn), command(0)
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
    ASSERT(cmd);
    ASSERT(cmd->cmd);
    ASSERT(cmd->done);
    PO_DEBUG("cmd=%p at='%s'", cmd, cmd->cmd);

    commands.append(cmd, mutex);
    cmd_sem->post();
}

void ESP8266::run_command()
{
    Command *cmd = commands.pop(mutex);
    ASSERT(cmd);
    ASSERT(cmd->done);
    PO_DEBUG("cmd=%p at=%s", cmd, cmd->cmd);

    if (hook)
    {
        hook->on_command(cmd);
    }

    send_at(cmd->cmd);

    // set the current command
    command = cmd;
}

    /*
     *
     */

class AtCommand : public ESP8266::Command
{
protected:
    ESP8266 *radio;
public:
    AtCommand(ESP8266 *_radio, const char *at)
    : ESP8266::Command(0, at), radio(_radio)
    {
        done = Semaphore::create();
    }
    ~AtCommand()
    {
        delete done;
    }

    void run()
    {
        radio->push_command(this);
        done->wait();
    }

    virtual bool process(const uint8_t *text)
    {
        // Check for completion of the command
        if (!strcmp((const char*) text, "OK"))
        {
            PO_DEBUG("OK cmd=%p", this);
            result = Command::OK;
            done->post();
            return true;
        }

        // TODO : handle ERR cases
        if (!strcmp((const char*) text, "ERROR"))
        {
            PO_ERROR("failed");
            return true;
        }

        return false;
    }
};

bool ESP8266::start()
{
    AtCommand cmd(this, "AT\r\n");

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

class Connect : public AtCommand
{
    char buff[64];
public:

    Connect(ESP8266 *radio, const char *ip, int port)
    : AtCommand(radio, 0)
    {
        snprintf(buff, sizeof(buff), "AT+CIPSTART=\"TCP\",\"%s\",%d\r\n", ip, port);
        cmd = buff;
    }
};

int ESP8266::connect(const char *ip, int port)
{
    Connect cmd(this, ip, port);
    cmd.run();
    if (cmd.result != Command::OK)
    {
        PO_ERROR("failed to place in Client mode");
        return 0;
    }

    return 1;
}

    /*
     *
     *
     */

class Send : public AtCommand
{
    char buff[32];
    const uint8_t *data;
    int size;
    bool okay;
    int prompt;
public:

    Send(ESP8266 *radio, const uint8_t *_data, int _size)
    : AtCommand(radio, 0), data(_data), size(_size), okay(false), prompt(0)
    {
        snprintf(buff, sizeof(buff), "AT+CIPSEND=%d\r\n", size);
        cmd = buff;
    }

    virtual bool process(const uint8_t *text)
    {
        // need to overide "OK" response, as that is not the end ..
        if (!strcmp((const char*) text, "OK"))
        {
            okay = true;
            return false;
        }

        if (AtCommand::process(text))
        {
            return true;
        }

        if (!strcmp((const char*) text, "SEND OK"))
        {
            result = Command::OK;
            done->post();
            return true;
        }

        // TODO
        return false;
    }

    virtual bool process(uint8_t c)
    {
        if (!okay)
        {
            // still waiting for 'OK'
            return false;
        }

        if (prompt > 1)
        {
            return false;
        }

        if ((prompt == 0) && (c == '>'))
        {
            prompt = 1;
            return true;
        }
        if ((prompt == 1) && (c == ' '))
        {
            prompt = 2;
            PO_DEBUG("send data ..");
            radio->send(data, size);
            return true;
        }

        PO_ERROR("%02x", c);
        return false;
    }
};

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

class CWJAP : public AtCommand
{
public:

    char buff[64];
    bool connected, ip;

    CWJAP(ESP8266 *radio, const char *ssid, const char *pw)
    : AtCommand(radio, 0), connected(false), ip(false)
    {
        snprintf(buff, sizeof(buff), "AT+CWJAP_DEF=\"%s\",\"%s\"\r\n", ssid, pw);
        cmd = buff;
    }

    virtual bool process(const uint8_t *text)
    {
        if (AtCommand::process(text))
        {
            return true;
        }

        PO_DEBUG("%s", text);

        if (!strcmp((const char*) text, "WIFI CONNECTED"))
        {
            connected = true;
            return false;
        }
        if (!strcmp((const char*) text, "WIFI DISCONNECT"))
        {
            connected = false;
            return false;
        }
        if (!strcmp((const char*) text, "WIFI GOT IP"))
        {
            ip = true;
            return false;
        }
        if (!strcmp((const char*) text, "FAIL"))
        {
            PO_ERROR("Failed");
            return true;
        }

        return false;
    }
};

    /*
     *
     */

bool ESP8266::connect_to_ap(const char* ssid, const char *pw)
{
    // Set the unit in WiFi client mode
    AtCommand cwmode(this, "AT+CWMODE=1\r\n");

    cwmode.run();
    if (cwmode.result != Command::OK)
    {
        PO_ERROR("failed to place in Client mode");
        return false;
    }

    // Set the access point's ssid and password
    CWJAP cwjap(this, ssid, pw);

    cwjap.run();
    if (cwjap.result != Command::OK)
    {
        PO_ERROR("failed to connect to AP");
        return false;
    }

    if (cwjap.connected)
    {
        PO_REPORT("wifi connected");
    }
    if (cwjap.ip)
    {
        PO_REPORT("wifi has ip address");
    }

    return cwjap.connected && cwjap.ip;
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

int ESP8266::send(const uint8_t *data, int size)
{
    PO_DEBUG("");

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
}

void ESP8266::process(const uint8_t *text)
{
    ASSERT(text);

    if (!strlen((const char*) text))
    {
        return;
    }

    PO_DEBUG("cmd=%p at='%s'", command, text);

    if (command)
    {
        if (command->process(text))
        {
            PO_DEBUG("end command %p", command);
            command = 0;
        }
    }
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

    if (command)
    {
        if (command->process(data))
        {
            // command wants to consume this char
            PO_DEBUG("consume %02x", data);
            return;
        }
    }

    //PO_DEBUG("%02x", data);
    buff[in] = data;
    in = next;
}

    /*
     *
     */

void ESP8266::run()
{
    PO_DEBUG("");

    reset();

    Select select(100);

    select.add(rd_sem);
    select.add(wait_sem);
    select.add(cmd_sem);

    is_running = true;

    timer_t last_tx = timer_now();

    while (!dead)
    {
        Semaphore *s = select.wait(& event_queue, 120000);

        if (s == cmd_sem)
        {
            run_command();
            continue;
        }

        if (s != rd_sem)
        {
            timer_t now = timer_now();

            if ((now - last_tx) > (120000 * 10))
            {
                //if (!command)
                {
                    //send_at("AT\r\n");
                    last_tx = now;
                }
            }

            continue;
        }

        // read the data ..
        uint8_t buff[128];
        const int n = rb->get(buff, sizeof(buff)-1);
        ASSERT((n >= 0) && (n <= (int)sizeof(buff)));

        for (int i = 0; i < n; i++)
        {
            //PO_DEBUG("%d %#02x", i, buff[i]);
            process(buff[i]);
        }
    }
}

}   //  namespace panglos

//  FIN
