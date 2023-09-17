
#include <stdint.h>

#include "panglos/drivers/uart.h"

namespace panglos {

class MHZ19B {
    UART *uart;

public:

    enum CMD { START=0xff, READ=0x86, ZERO=0x87, CAL=0x79, RANGE=0x99 };

    MHZ19B(UART *uart);

    static uint8_t checksum(const uint8_t *data);

    bool request();

    struct Data {
        uint16_t co2;
    };

    bool read(struct Data *data);
};

}   //  namespace panglos

//  FIN
