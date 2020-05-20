
#include <string.h>
#include <stdio.h>
#include <ctype.h>

#include "panglos/debug.h"
#include "panglos/uart.h"
#include "panglos/radio.h"
#include "panglos/event.h"

namespace panglos {

Radio::Radio(Output *_out, RdBuff *_rd, Semaphore *_rx_sem, GPIO *r)
: out(_out), rd(_rd), select(), rx_sem(_rx_sem), timeout_sem(0), reset(r), reading(0), in(0)
{
    timeout_sem = Semaphore::create();

    select = new Select(3);
    select->add(rx_sem);
    select->add(timeout_sem);
}

Radio::~Radio()
{
    delete select;
    delete timeout_sem;
}

class AutoEvent : public Event
{
    Semaphore *s;
public:
    AutoEvent(Semaphore *_s, timer_t period)
    :   Event(_s, period + timer_now()), s(_s)
    {
        event_queue.add(this);
    }
    ~AutoEvent()
    {
        event_queue.remove(this);
    }
};

int Radio::init()
{
    if (!reset)
    {
        // currently must have a hard reset
        ASSERT(0);
    }

    Semaphore *s = Semaphore::create();

    PO_DEBUG("reset GPIO");
    reset->set(false);
    event_queue.wait(s, TIMER_MS * 500);
    reset->set(true);
    event_queue.wait(s, TIMER_MS * 500);

    rd->reset();
    in = 0;

    delete s;
    return 0;
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

int Radio::create_reader(const char *idp)
{
    PO_DEBUG("%s", idp);

    ASSERT(!strncmp("+IPD,", idp, 5));

    // read the <length> field
    int size = 0;
    for (const char *s = & idp[5]; *s != ':'; s++)
    {
        ASSERT(*s);
        size *= 10;
        if (!isdigit(*s))
        {
            return 0;
        }
        size += *s - '0';
    }

    // sanity check on size
    if ((size < 0) || (size > 1024))
    {
        PO_ERROR("size=%d too big", size);
        return 0;
    }

    // allocate read buffer
    buffers.add_buffer(size);

    return size;
}

bool Radio::process(char c)
{
    // if reading data from +IPD, just do that
    if (reading)
    {
        buffers.add(c);
        reading -= 1;
        if (reading == 0)
        {
            PO_DEBUG("done read");
        }
        return false;
    }

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

    // match +IPD input
    if ((in > 5) && (c == ':') && !strncmp("+IPD,", buff, 5))
    {
        buff[in] = '\0';
        reading = create_reader(buff);
        in = 0;
        return false;
    }

    return false;
}

int Radio::read_line(char *data, int size)
{
    while (true)
    {
        Semaphore *s = select->wait();
        //PO_DEBUG("sem=%p", s);
        if (s == timeout_sem)
        {
            PO_DEBUG("timeout");
            return 0;
        }
        //ASSERT(s == rx_sem);

        while (true)
        {
            char c;
            if (!rd->get((uint8_t*) & c, 1))
            {
                break;
            }
            if (process(c))
            {
                PO_DEBUG("%s reading=%d", buff, reading);
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
        //PO_DEBUG("sem=%p", s);
        if (s == timeout_sem)
        {
            PO_DEBUG("timeout");
            return false;
        }
        //ASSERT(s == rx_sem);

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

    /*
     *
     */

int Radio::set_ap(bool on, timer_t timeout)
{
    char buff[128];

    snprintf(buff, sizeof(buff), "AT+CWMODE=%s\r\n", on ? "2" : "1");

    send_at(buff);
    buff[0] = '\0';

    AutoEvent period(timeout_sem, timeout);

    while (true)
    {
        if (!read_line(buff, sizeof(buff)))
        {
            PO_ERROR("timeout");
            return -1;
        }

        if (!strncmp(buff, "AT+CWMODE=", 10))
        {
            // echo
            continue;
        }
        if (!strcmp(buff, "FAIL"))
        {
            return -1;
        }
        if (!strcmp(buff, "OK"))
        {
            return 0;
        }

        // unknown response
        PO_ERROR("unknown: '%s'", buff);
    }
}

int Radio::connect(const char *ssid, const char *pw, timer_t timeout)
{
    char buff[128];

    AutoEvent period(timeout_sem, timeout);

    snprintf(buff, sizeof(buff), "AT+CWJAP=\"%s\",\"%s\"\r\n", ssid, pw);

    send_at(buff);
    buff[0] = '\0';

    bool connected = false, ip = false;

    while (true)
    {
        if (!read_line(buff, sizeof(buff)))
        {
            PO_ERROR("timeout");
            return -1;
        }

        if (!strncmp(buff, "AT+CWJAP=", 9))
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
            return -1;
        }
        if (!strcmp(buff, "ERROR"))
        {
            return -1;
        }
        if (!strcmp(buff, "OK"))
        {
            return (connected && ip) ? 0 : -1;
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
        if (!strcmp(buff, "link is not valid"))
        {
            continue;
        }
        if (!strcmp(buff, "ERROR"))
        {
            return -1;
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

        if (!strncmp(buff, "Recv ", 5))
        {
            // "Recv nn bytes"
            continue;
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
    int count = buffers.read((uint8_t*) data, size);

    if (count == size)
    {
        return count;
    }

    //PO_DEBUG("size=%d reading=%d", size, reading);

    AutoEvent period(timeout_sem, reading ? (TIMER_S * 2) : timeout);
    //AutoEvent period(timeout_sem, timeout);

    while (true)
    {
        Semaphore *s = select->wait();
        if (s == timeout_sem)
        {
            //PO_DEBUG("timeout");
            if (rd->empty())
            {
                return count;
            }
        }

        while (true)
        {
            char c;
            if (!rd->get((uint8_t*) & c, 1))
            {
                break;
            }
            process(c);
            rx_sem->post();

            const int more = size - count;
            count += buffers.read((uint8_t*) & data[count], more);

            if (count == size)
            {
                return count;
            }
        }
    }
}

void Radio::flush_read()
{
    buffers.reset();
    reading = 0;
}

}   //  namespace panglos

//  FIN
