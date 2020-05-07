
#if !defined(__ESP8266_CMD_H__)
#define __ESP8266_CMD_H__

namespace panglos {

    /*
     *
     */

class ESP8266::Command
{
public:

    enum Result {
        OK = 0,
        ERR,
    };

    Command *next;
    const char *at;
    Result result;
    const char *name;

    Command(const char* _at, const char *name)
    : next(0), at(_at), result(ERR), name(name)
    {
    }

    virtual ~Command() { }

    virtual void start() { }
    virtual bool process(const uint8_t *line) = 0;
    virtual bool process(uint8_t c) { IGNORE(c); return false; };
};

    /*
     *
     */

class AtCommand : public ESP8266::Command
{
protected:
    ESP8266 *radio;
    Semaphore *done;
public:
    AtCommand(ESP8266 *_radio, const char *at, const char *name)
    : ESP8266::Command(at, name), radio(_radio), done(0)
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
        at = buff;
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

    Send(ESP8266 *radio, const uint8_t *_data, int _size)
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
        at = buff;
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
        }
        return true;
    }
};

}   //  namespace panglos

#endif // __ESP8266_CMD_H__

//  FIN
