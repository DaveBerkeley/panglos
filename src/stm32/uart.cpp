
#include <string.h>

#if defined(STM32F4)

#include "panglos/debug.h"
#undef UNUSED

#include "panglos/stm32/uart.h"
#include "panglos/stm32/gpio_arm.h"

static void uart_enable_clock(USART_TypeDef *uart)
{
    // stm32f446 : USART1 USART2 USART3 UART4 UART5 USART6
    ASSERT(uart);
    if (uart == USART1) { __HAL_RCC_USART1_CLK_ENABLE(); return; }
    if (uart == USART2) { __HAL_RCC_USART2_CLK_ENABLE(); return; }
    if (uart == USART3) { __HAL_RCC_USART3_CLK_ENABLE(); return; }
#if defined(USART4)
    if (uart == USART4) { __HAL_RCC_USART4_CLK_ENABLE(); return; }
#endif
#if defined(USART5)
    if (uart == USART5) { __HAL_RCC_USART5_CLK_ENABLE(); return; }
#endif
    if (uart == USART6) { __HAL_RCC_USART6_CLK_ENABLE(); return; }
    ASSERT(0);
}

    /*
     *
     */

namespace panglos {

void STM32_UART::init_gpio(struct Config::Pin *pin)
{
    gpio_enable_clock(pin->port);

    GPIO_InitTypeDef config = {0};

    config.Pin = pin->pin;
    config.Mode = GPIO_MODE_AF_PP;
    config.Pull = GPIO_NOPULL ; // GPIO_PULLUP;
    config.Speed = GPIO_SPEED_FREQ_HIGH;
    config.Alternate = pin->alt;
    HAL_GPIO_Init(pin->port, & config);
}

bool STM32_UART::init_uart(USART_TypeDef *uart, int baud)
{
    handle.Instance = uart;  // Specify which USART peripheral to use
    handle.Init.BaudRate = baud; // Set baud rate
    handle.Init.WordLength = UART_WORDLENGTH_8B; // 8 data bits
    handle.Init.StopBits = UART_STOPBITS_1;      // 1 stop bit
    handle.Init.Parity = UART_PARITY_NONE;       // No parity
    handle.Init.Mode = UART_MODE_TX_RX;          // Enable both TX and RX
    handle.Init.HwFlowCtl = UART_HWCONTROL_NONE; // No hardware flow control
    handle.Init.OverSampling = UART_OVERSAMPLING_16; // Oversampling setting [citation:5][citation:11]

    // Initialize the UART with the configured parameters
    HAL_StatusTypeDef ok = HAL_UART_Init(& handle);
    // show error?
    /* Optionally configure and enable the UART interrupt */
    // HAL_NVIC_SetPriority(USART1_IRQn, 0, 0);
    // HAL_NVIC_EnableIRQ(USART1_IRQn);
    return ok == HAL_OK;
}

STM32_UART::STM32_UART()
{
    memset(& handle, 0, sizeof(handle));
}

bool STM32_UART::init(struct Config *config)
{
    uart_enable_clock(config->uart);
    init_gpio(& config->rx);
    init_gpio(& config->tx);
    return init_uart(config->uart, config->baud);
}

int STM32_UART::rx(char* data, int n)
{
    // TODO
    ASSERT(0);
    UNUSED(data);
    UNUSED(n);
    return 0;
}

int STM32_UART::tx(const char* data, int n)
{
    HAL_StatusTypeDef ok = HAL_UART_Transmit(& handle, (const uint8_t*) data, n, HAL_MAX_DELAY);    
    if (ok != HAL_OK)
    {
        // TODO : ERROR!
    }
    return (ok == HAL_OK) ? n : 0;
}     

#endif  //  STM32F4

    /*
     *
     */

STM32_UART *STM32_UART::create(STM32_UART::Config *config)
{
    ASSERT(config);

    STM32_UART *uart = new STM32_UART;

    if (uart->init(config))
    {
        return uart;
    }
 
    // ERROR message
    delete uart;
    return 0;
}

}   //  namespace panglos

//  FIN
