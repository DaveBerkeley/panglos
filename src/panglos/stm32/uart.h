
#include "panglos/stm32/hal.h"
#include "panglos/stm32/dma.h"
#include "panglos/stm32/gpio_arm.h"

#include "panglos/drivers/uart.h"

namespace panglos {

struct UartConfig
{
    struct Pin rx;
    struct Pin tx;

    USART_TypeDef *uart;
    int baud;
    bool rx_irq;
    bool tx_irq;

    struct Dma::Config rx_dma;
    struct Dma::Config tx_dma;

    static IRQn_Type get_irq_type(USART_TypeDef *uart);
};

UART *create_uart(const struct UartConfig *config);

}   //  namespace panglos

//  FIN
