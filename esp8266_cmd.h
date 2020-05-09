
#if !defined(__ESP8266_CMD_H__)
#define __ESP8266_CMD_H__

namespace panglos {

    /*
     *
     */

class Radio::Command
{
public:

    enum Result {
        OK = 0,
        ERR,
        INIT,
    };

    Command *next;
    const char *at;
    Result result;
    const char *name;
    bool cancelled;
    bool active;

    Command(const char* _at, const char *name)
    : next(0), at(_at), result(INIT), name(name), cancelled(false), active(true)
    {
        PO_DEBUG("cmd=%p at='%s' %s", this, at, name);
    }

    virtual ~Command() { PO_DEBUG("cmd=%p", this); }

    virtual void start() { }
    virtual bool process(const uint8_t *line) = 0;
    virtual bool consume(uint8_t c) { IGNORE(c); return false; };

    static Command **next_fn(Command *cmd) { return & cmd->next; }
};

    /*
     *
     */

class AtCommand : public Radio::Command
{
protected:
    Radio *radio;
    Semaphore *done;
public:
    AtCommand(Radio *_radio, const char *at, const char *name)
    : Radio::Command(at, name), radio(_radio), done(0)
    {
        done = Semaphore::create();
    }
    virtual ~AtCommand()
    {
        radio->check(this);
        delete done;
    }

    void run()
    {
        radio->request_command(this);
        done->wait();
    }

    virtual bool process(const uint8_t *text)
    {
        // Check for completion of the command
        if (!strcmp((const char*) text, "OK"))
        {
            PO_DEBUG("OK cmd=%s %p", name, this);
            result = Command::OK;
            radio->end_command(this, done);
            return true;
        }

        // TODO : handle ERR cases
        if (!strcmp((const char*) text, "ERROR"))
        {
            PO_ERROR("failed");
            result = Command::ERR;
            radio->end_command(this, done);
            return true;
        }

        return false;
    }
};

    /*
     *
     */

class Connect : public AtCommand
{
    char buff[64];
public:
    bool connected;

    Connect(Radio *radio, const char *ip, int port, Radio::Transport transport)
    : AtCommand(radio, 0, "Connect"), connected(false)
    {
        const char *tcp = (transport == Radio::TCP) ? "TCP" : "UDP";
        snprintf(buff, sizeof(buff), "AT+CIPSTART=\"%s\",\"%s\",%d\r\n", tcp, ip, port);
        at = buff;
    }

    virtual bool process(const uint8_t *text)
    {
        if (AtCommand::process(text))
        {
            return true;
        }
        if (!strcmp((const char*) text, "CONNECT"))
        {
            connected = true;
        }
        return false;
    }
};

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

    Send(Radio *radio, const uint8_t *_data, int _size)
    : AtCommand(radio, 0, "Send"), data(_data), size(_size), okay(false), prompt(0)
    {
        snprintf(buff, sizeof(buff), "AT+CIPSEND=%d\r\n", size);
        at = buff;
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
            radio->end_command(this, done);
            return true;
        }

        return false;
    }

    virtual bool consume(uint8_t c)
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
            radio->write(data, size);
            return true;
        }

        PO_ERROR("%02x", c);
        prompt = 0;
        return false;
    }
};

    /*
     *
     */

class ConnectAp : public AtCommand
{
public:

    char buff[64];
    bool connected, ip;

    ConnectAp(Radio *radio, const char *ssid, const char *pw)
    : AtCommand(radio, 0, "ConnectAp"), connected(false), ip(false)
    {
        snprintf(buff, sizeof(buff), "AT+CWJAP_DEF=\"%s\",\"%s\"\r\n", ssid, pw);
        at = buff;
    }

    virtual bool process(const uint8_t *text)
    {
        if (AtCommand::process(text))
        {
            return true;
        }

        //PO_DEBUG("%s", text);

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
            radio->end_command(this, done);
            return true;
        }

        return false;
    }
};

    /*
     *
     */

class Read : public AtCommand
{
    Semaphore *semaphore;
    uint8_t *buffer;
    int len, *count;
public:

    Read(Radio *radio, Semaphore *s, uint8_t *_buffer, int _len, int *_count)
    : AtCommand(radio, 0, "Read"), semaphore(s), buffer(_buffer), len(_len), count(_count)
    {
    }

    virtual void start()
    {
        //  read the data ...
        const int n = radio->read(buffer, len);
        *count += n;
        len -= n;
        buffer += n;
        if (len == 0)
        {
            result = Command::OK;
            radio->end_command(this, semaphore);
        }
    }

    virtual bool consume(uint8_t c)
    {
        ASSERT(len);
        //still some more data to read
        *buffer++ = c;
        len -= 1;
        if (len == 0)
        {
            result = Command::OK;
            radio->end_command(this, semaphore);
        }
        return true;
    }
};

}   //  namespace panglos

#endif // __ESP8266_CMD_H__

//  FIN
