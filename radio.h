
#if !defined(__RADIO_H__)
#define __RADIO_H__

#include "uart.h"
#include "timer.h"
#include "select.h"

namespace panglos {

class Radio
{
public:
    typedef UART::Buffer RdBuff;

private:
    Output *out;
    RdBuff *rd;
    Select *select;
    Semaphore *rx_sem;
    Semaphore *timeout_sem;

    int read_line(char *buff, int size);
    bool wait_for(const char *buff, int size);
    int send(const char *data, int size);
    int send_at(const char *at);

    bool process(char c);

public:
    Radio(Output *out, RdBuff *rd, Semaphore *rx_sem);
    ~Radio();

    bool connect(const char *ssid, const char *pw, timer_t timeout);
    int socket_open(const char *host, int port, timer_t timeout);
    int socket_send(const char *data, int size, timer_t timeout);
};

}   //  namespace panglos

#endif // __RADIO_H__

//  FIN
