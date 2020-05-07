
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>

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
    mutex(0), hook(0), commands(next_fn), delete_queue(next_fn), command(0), reading(false)
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
    ASSERT(cmd->done);
    //PO_DEBUG("cmd=%p at='%s'", cmd, cmd->cmd);

    commands.append(cmd, mutex);
    cmd_sem->post();
}

void ESP8266::push_delete(Command *cmd)
{
    ASSERT(cmd);
    //PO_DEBUG("cmd=%s %p", cmd->name, cmd);
    delete_queue.append(cmd, mutex);
}

void ESP8266::run_command()
{
    Command *cmd = commands.pop(mutex);
    ASSERT(cmd);
    ASSERT(cmd->done);
    //PO_DEBUG("cmd=%p at=%s", cmd, cmd->cmd);

    // set the current command
    command = cmd;

    cmd->start();

    if (hook)
    {
        hook->on_command(cmd);
    }

    if (cmd->cmd)
    {
        send_at(cmd->cmd);
    }
}

    /*
     *
     */

class AtCommand : public ESP8266::Command
{
protected:
    ESP8266 *radio;
public:
    AtCommand(ESP8266 *_radio, const char *at, const char *name)
    : ESP8266::Command(0, at, name), radio(_radio)
    {
        done = Semaphore::create();
    }
    virtual ~AtCommand()
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
            PO_DEBUG("OK cmd=%s %p", name, this);
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

class Connect : public AtCommand
{
    char buff[64];
public:

    Connect(ESP8266 *radio, const char *ip, int port)
    : AtCommand(radio, 0, "Connect")
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
    : AtCommand(radio, 0, "Send"), data(_data), size(_size), okay(false), prompt(0)
    {
        snprintf(buff, sizeof(buff), "AT+CIPSEND=%d\r\n", size);
        cmd = buff;
    }

    virtual bool process(const uint8_t *text)
    {
        // need to override "OK" response, as that is not the end ..
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

        // wait for '> ' to before sending data
        if ((prompt == 0) && (c == '>'))
        {
            prompt = 1;
            return true;
        }
        if ((prompt == 1) && (c == ' '))
        {
            prompt = 2;
            radio->send(data, size);
            return true;
        }

        PO_ERROR("%02x", c);
        prompt = 0;
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
    : AtCommand(radio, 0, "CWJAP"), connected(false), ip(false)
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
    AtCommand cwmode(this, "AT+CWMODE=1\r\n", "CWMODE");

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

    /*
     *
     */

class Read : public AtCommand
{
    Semaphore *semaphore;
    uint8_t *buffer;
    int len, *count;
public:

    Read(ESP8266 *radio, Semaphore *s, uint8_t *_buffer, int _len, int *_count)
    : AtCommand(radio, 0, "Read"), semaphore(s), buffer(_buffer), len(_len), count(_count)
    {
    }

    virtual void start()
    {
        //  read the data ...
        const int n = radio->buffers.read(buffer, len);
        *count += n;
        len -= n;
        buffer += n;
        if (len == 0)
        {
            radio->end_command();
            semaphore->post();
            //radio->push_delete(this);
        }
    }

    virtual bool process(uint8_t c)
    {
        ASSERT(len);
        //still some more data to read
        *buffer++ = c;
        len -= 1;
        if (len == 0)
        {
            radio->end_command();
            semaphore->post();
            //radio->push_delete(this);
        }
        return true;
    }
};

ESP8266::Command *ESP8266::read(Semaphore *s, uint8_t *buffer, int len, int *count)
{
    Read *read = new Read(this, s, buffer, len, count);
    push_command(read);
    return read;
}

void ESP8266::cancel(Command *cmd)
{
    //PO_DEBUG("%s %p", cmd->name, cmd);
    if (command == cmd)
    {
        //PO_DEBUG("end command");
        command = 0;
    }
    push_delete(cmd);
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
    event_queue.wait(s, 50000);
    gpio_reset->set(true);
    event_queue.wait(s, 10000);

    rb->reset();
    in = 0;

    PO_DEBUG("reset");
    delete s;
}

    /*
     *
     */

int ESP8266::send(const uint8_t *data, int size)
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
}

void ESP8266::end_command()
{
    PO_DEBUG("end command %s %p", command->name, command);
    command = 0;
}

void ESP8266::process(const uint8_t *text)
{
    ASSERT(text);

    if (!strlen((const char*) text))
    {
        return;
    }

    PO_DEBUG("cmd=%s %p at='%s'", command->name, command, text);

    if (command)
    {
        if (command->process(text))
        {
            end_command();
        }
    }
}

void ESP8266::process(uint8_t data)
{
    if (command)
    {
        if (command->process(data))
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

void ESP8266::create_rx_buffer(uint8_t *ipd)
{
    // should have "+IPD,<length>:"
    //PO_DEBUG("got %s", ipd);

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

    //PO_DEBUG("size=%d", size);

    // rationality check
    if ((size < 0) || (size > 1024))
    {
        PO_ERROR("bad buffer size=%d", size);
        return;
    }

    // Allocate read buff
    buffers.add_buffer(size);
    // redirect reads until done
    reading = true;
    //PO_DEBUG("reading=%d", reading);
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

    while (!dead)
    {
        Semaphore *s = select.wait(& event_queue, 120000);

        if (s == cmd_sem)
        {
            run_command();
            continue;
        }

        // delete any old messages
        while (!delete_queue.empty())
        {
            Command *c = delete_queue.pop(mutex);
            PO_DEBUG("delete %s %p", c->name, c);
            delete c;
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
