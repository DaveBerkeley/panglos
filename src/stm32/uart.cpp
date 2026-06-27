
#include <string.h>

#if defined(STM32F4)

#include "panglos/debug.h"
#undef UNUSED

#include "panglos/stm32/uart.h"

static void init_uart_clk(USART_TypeDef *uart)
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
     *  These functions should be part of the GPIO library
     */ 

static void init_gpio_clk(GPIO_TypeDef *port)
{
    ASSERT(port);
    // stm32f446 : GPIOA GPIOB GPIOC GPIOD GPIOE GPIOF GPIOG GPIOH
    if (port == GPIOA) { __HAL_RCC_GPIOA_CLK_ENABLE(); return; }
    if (port == GPIOB) { __HAL_RCC_GPIOB_CLK_ENABLE(); return; }
    if (port == GPIOC) { __HAL_RCC_GPIOC_CLK_ENABLE(); return; }
    if (port == GPIOD) { __HAL_RCC_GPIOD_CLK_ENABLE(); return; }
    if (port == GPIOE) { __HAL_RCC_GPIOE_CLK_ENABLE(); return; }
    if (port == GPIOF) { __HAL_RCC_GPIOF_CLK_ENABLE(); return; }
    if (port == GPIOG) { __HAL_RCC_GPIOG_CLK_ENABLE(); return; }
    if (port == GPIOH) { __HAL_RCC_GPIOH_CLK_ENABLE(); return; }
    ASSERT(0);
}

static void _init_gpio(GPIO_TypeDef *port, GPIO_InitTypeDef *config)
{
    init_gpio_clk(port);
    HAL_GPIO_Init(port, config);
}

    /*
     *
     */

namespace panglos {

void STM32_UART::init_gpio(struct Config::Pin *pin)
{
    GPIO_InitTypeDef config = {0};

    config.Pin = pin->pin;
    config.Mode = GPIO_MODE_AF_PP;
    config.Pull = GPIO_NOPULL ; // GPIO_PULLUP;
    config.Speed = GPIO_SPEED_FREQ_HIGH;
    config.Alternate = pin->alt;
    _init_gpio(pin->port, & config);
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
    init_uart_clk(config->uart);
    init_gpio(& config->rx);
    init_gpio(& config->tx);
    return init_uart(config->uart, config->baud);
}

int STM32_UART::rx(char* data, int n)
{
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
