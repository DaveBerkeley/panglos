
#if !defined(__ESP8266_H__)
#define __ESP8266_H__

#include "uart.h"
#include "buffer.h"
#include "gpio.h"

namespace panglos {

class ESP8266
{
    UART *uart;
    RingBuffer *rb;
    GPIO *gpio_reset;
    Semaphore *semaphore;

    void reset();
public:
    ESP8266(UART *uart, RingBuffer *b, GPIO *reset);
    ~ESP8266();

    void run();
};

}   //  namespace panglos

#endif // __ESP8266_H__

// FIN
