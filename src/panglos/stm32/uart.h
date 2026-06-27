
#include "panglos/stm32/hal.h"

#include "panglos/drivers/uart.h"

namespace panglos {

class STM32_UART : public panglos::UART
{
    UART_HandleTypeDef handle;

public:
    struct Config
    {
        struct Pin {
            GPIO_TypeDef *port;
            uint32_t    pin;
            uint32_t    alt;
        };

        struct Pin rx;
        struct Pin tx;

        USART_TypeDef *uart;
        int baud;
    };

private:
    bool init_uart(USART_TypeDef *uart, int baud);
    void init_gpio(struct Config::Pin *pin);

public:
    STM32_UART();

    bool init(struct Config *config);

    // Implement UART
    virtual int rx(char* data, int n) override;
    virtual int tx(const char* data, int n) override;

    static STM32_UART *create(struct Config *config);
};

}   //  namespace panglos

//  FIN
