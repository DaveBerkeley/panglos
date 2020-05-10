
#include <string.h>

#include "debug.h"
#include "uart.h"
#include "radio.h"
#include "event.h"

namespace panglos {

Radio::Radio(Output *_out, RdBuff *_rd, Semaphore *_rx_sem)
: out(_out), rd(_rd), select(), rx_sem(_rx_sem), timeout_sem(0)
{
    timeout_sem = Semaphore::create();

    select = new Select(4);
    select->add(rx_sem);
    select->add(timeout_sem);
}

Radio::~Radio()
{
    delete select;
    delete timeout_sem;
}

int Radio::send(const char *data, int size)
{
    ASSERT(out);
    int count = 0;
    for (int i = 0; i < size; i++)
    {
        count += out->_putc(*data++);
    }
    return count;
}

int Radio::send_at(const char *at)
{
    ASSERT(out);
    return out->_puts(at);
}

static char buff[128];
static int in = 0;

bool Radio::process(char c)
{
    const int next = in + 1;

    if (next == sizeof(buff))
    {
        PO_ERROR("Overflow");
        in = 0;
        return false;
    }

    if (c == '\r')
    {
        buff[in] = '\0';
        return in > 0;
    }
    if (c == '\n')
    {
        // ignore \n
        return false;
    }

    buff[in] = c;
    in = next;

    // TODO : match +IDP input

    return false;
}

int Radio::read_line(char *data, int size)
{
    while (true)
    {
        Semaphore *s = select->wait();
        PO_DEBUG("sem=%p", s);
        if (s == timeout_sem)
        {
            PO_DEBUG("timeout");
            return 0;
        }
        ASSERT(s == rx_sem);

        while (true)
        {
            char c;
            if (!rd->get((uint8_t*) & c, 1))
            {
                break;
            }
            if (process(c))
            {
                PO_DEBUG("%s", buff);
                int len = ((in+1) < size) ? in+1 : size;
                memcpy(data, buff, len);
                in = 0;
                // need to let select know we haven't finished reading ..
                rx_sem->post();
                return len;
            }
        }
    }

    return 0;
}

bool Radio::wait_for(const char *data, int size)
{
    int in = 0;

    while (true)
    {
        Semaphore *s = select->wait();
        PO_DEBUG("sem=%p", s);
        if (s == timeout_sem)
        {
            PO_DEBUG("timeout");
            return false;
        }
        ASSERT(s == rx_sem);

        while (true)
        {
            char c;
            if (!rd->get((uint8_t*) & c, 1))
            {
                break;
            }
            if (c == data[in])
            {
                // match
                in += 1;
                if (in == size)
                {
                    PO_DEBUG("match '%s'", data);
                    rx_sem->post();
                    return true;
                }
            }
            else
            {
                // no match. start again
                in = 0;
            }
        }
    }

    return 0;
}

class AutoEvent : public Event
{
public:
    AutoEvent(Semaphore *s, timer_t period)
    :   Event(s, period + timer_now())
    {
        event_queue.add(this);
    }
    ~AutoEvent()
    {
        event_queue.remove(this);
    }
};

    /*
     *
     */

bool Radio::connect(const char *ssid, const char *pw, timer_t timeout)
{
    char buff[128];

    snprintf(buff, sizeof(buff), "AT+CWJAP_DEF=\"%s\",\"%s\"\r\n", ssid, pw);
    send_at(buff);
    buff[0] = '\0';

    bool connected = false, ip = false;

    AutoEvent period(timeout_sem, timeout);

    while (true)
    {
        if (!read_line(buff, sizeof(buff)))
        {
            PO_ERROR("timeout");
            return 0;
        }

        if (!strncmp(buff, "AT+CWJAP_DEF=", 13))
        {
            // echo
            continue;
        }
        if (!strcmp(buff, "WIFI CONNECTED"))
        {
            connected = true;
            continue;
        }
        if (!strcmp(buff, "WIFI DISCONNECT"))
        {
            connected = false;
            continue;
        }
        if (!strcmp(buff, "WIFI GOT IP"))
        {
            ip = true;
            continue;
        }
        if (!strcmp(buff, "FAIL"))
        {
            return false;
        }
        if (!strcmp(buff, "OK"))
        {
            return connected && ip;
        }

        // unknown response
        PO_ERROR("unknown: '%s'", buff);
    }
}

int Radio::socket_open(const char *host, int port, timer_t timeout)
{
    char buff[128];

    const char *tcp = "TCP";
    snprintf(buff, sizeof(buff), "AT+CIPSTART=\"%s\",\"%s\",%d\r\n", tcp, host, port);
    send_at(buff);
    buff[0] = '\0';

    bool connected = false;

    AutoEvent period(timeout_sem, timeout);

    while (true)
    {
        if (!read_line(buff, sizeof(buff)))
        {
            PO_ERROR("timeout");
            return 0;
        }

        if (!strncmp(buff, "AT+CIPSTART=", 12))
        {
            // echo
            continue;
        }
        if (!strcmp(buff, "CONNECT"))
        {
            connected = true;
            continue;
        }
        // TODO : check fail case
        if (!strcmp(buff, "FAIL"))
        {
            return false;
        }
        if (!strcmp(buff, "OK"))
        {
            return connected;
        }

        // unknown response
        PO_ERROR("unknown: '%s'", buff);
    }
    return 0;
}

int Radio::socket_send(const char *data, int size, timer_t timeout)
{
    char buff[128];

    snprintf(buff, sizeof(buff), "AT+CIPSEND=%d\r\n", size);
    send_at(buff);
    buff[0] = '\0';

    AutoEvent period(timeout_sem, timeout);

    while (true)
    {
        if (!read_line(buff, sizeof(buff)))
        {
            PO_ERROR("timeout");
            return 0;
        }

        if (!strncmp(buff, "AT+CIPSEND=", 11))
        {
            // echo
            continue;
        }
        // TODO : check fail case
        if (!strcmp(buff, "FAIL"))
        {
            return false;
        }
        if (!strcmp(buff, "OK"))
        {
            break; 
        }

        // unknown response
        PO_ERROR("unknown: '%s'", buff);
    }

    // Wait for "> " prompt
    if (!wait_for("> ", 2))
    {
        PO_ERROR("no '> ' prompt");
        return 0;
    }
    
    int n = send(data, size);
    if (n != size)
    {
        PO_ERROR("sent=%d size=%d", n, size);
        return 0;
    }

    // wait for OK
    while (true)
    {
        if (!read_line(buff, sizeof(buff)))
        {
            PO_ERROR("timeout");
            return 0;
        }

        // TODO : check fail case
        if (!strcmp(buff, "FAIL"))
        {
            return false;
        }
        if (!strcmp(buff, "SEND OK"))
        {
            return size;
        }

        // unknown response
        PO_ERROR("unknown: '%s'", buff);
    }
}

int Radio::socket_read(char *data, int size, timer_t timeout)
{
    IGNORE(data);
    IGNORE(size);
    IGNORE(timeout);
    return 0;
}

}   //  namespace panglos

//  FIN
