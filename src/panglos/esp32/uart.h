
#include "panglos/drivers/uart.h"

namespace panglos {

class ESP_UART : public panglos::UART
{
public:
    static ESP_UART *create(int id, uint32_t rx, uint32_t tx, uint32_t baud=115200, bool verbose=false);
};

}   //  namespace panglos

//  FIN
