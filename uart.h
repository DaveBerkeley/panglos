
#if !defined(__UART_H__)
#define __UART_H__

#include "buffer.h"
#include "sprintf.h"

namespace panglos {

    /*
     *
     */

class UART : public Output
{
public:
    enum Id {
        UART_1 = 0,
        UART_2 = 1,
        UART_3 = 2,
    };

    virtual ~UART() { }

    // implement Output interface
    virtual int _putc(char c) = 0;
    virtual int send(const char* data, int n) = 0;

    virtual uint32_t get_error() { return 0; }

    typedef RingBuffer<uint8_t> Buffer;

    static UART *create(Id id, int baud, Buffer *b);
};

}   //  namespace panglos

#endif // __UART_H__

//  FIN
