
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#include "stm32f4xx_hal.h"
#include "stm32f4xx_hal_uart.h"

#include "../debug.h"

#include "../gpio.h"
#include "../mutex.h"
#include "../list.h"
#include "../uart.h"

namespace panglos {

static UART_HandleTypeDef uart1;
static UART_HandleTypeDef uart2;
static UART_HandleTypeDef uart3;

class ArmUart;

static ArmUart *uarts[3];

static UART_HandleTypeDef *get_uart(panglos::UART::Id id)
{
    switch (id)
    {
        case panglos::UART::UART_1 :
        {
            return & uart1;
        }
        case panglos::UART::UART_2 :
        {
            return & uart2;
        }
        case panglos::UART::UART_3 :
        {
            return & uart3;
        }
    }

    ASSERT(0);
    return 0;
}

static IRQn_Type get_irq_num(panglos::UART::Id id)
{
    switch (id)
    {
        case panglos::UART::UART_1 :
        {
            return USART1_IRQn;
        }
        case panglos::UART::UART_2 :
        {
            return USART2_IRQn;
        }
        case panglos::UART::UART_3 :
        {
            return USART3_IRQn;
        }
    }

    ASSERT(0);
    return (IRQn_Type) 0;
}

    /*
     *
     */

static UART_HandleTypeDef* MX_UART_Init(panglos::UART::Id id, uint32_t baud)
{

    UART_HandleTypeDef *uart = get_uart(id);
    uint32_t rx_pin;
    uint32_t tx_pin;
    uint32_t alternate;
    USART_TypeDef* instance = 0;
    GPIO_TypeDef *port;
    uint32_t tx_mode;
    uint32_t rx_mode;

    switch (id)
    {
        case panglos::UART::UART_1 :
        {
            __USART1_CLK_ENABLE();
            tx_pin = GPIO_PIN_9;
            rx_pin = GPIO_PIN_10;
            alternate = GPIO_AF7_USART1;
            instance = USART1;
            port = GPIOA;
            tx_mode = GPIO_MODE_AF_PP;
            rx_mode = GPIO_MODE_AF_PP;
            break;
        }
        case panglos::UART::UART_2 :
        {
            __USART2_CLK_ENABLE();
            tx_pin = GPIO_PIN_2;
            rx_pin = GPIO_PIN_3;
            alternate = GPIO_AF7_USART2;
            instance = USART2;
            port = GPIOA;
            tx_mode = GPIO_MODE_AF_PP;
            rx_mode = GPIO_MODE_AF_PP;
            break;
        }
        case panglos::UART::UART_3 :
        {
            __USART3_CLK_ENABLE();
            tx_pin = GPIO_PIN_10;
            rx_pin = GPIO_PIN_11;
            alternate = GPIO_AF7_USART3;
            instance = USART3;
            port = GPIOB;
            tx_mode = GPIO_MODE_AF_PP;
            rx_mode = GPIO_MODE_AF_PP;
            break;
        }
        default :
        {
            ASSERT(0);
        }
    }

    GPIO_InitTypeDef gpio_def;

    gpio_def.Pin = tx_pin;
    gpio_def.Mode = tx_mode;
    gpio_def.Alternate = alternate;
    gpio_def.Speed = GPIO_SPEED_HIGH;
    gpio_def.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(port, & gpio_def);

    gpio_def.Pin = rx_pin;
    gpio_def.Mode = rx_mode;
    HAL_GPIO_Init(port, & gpio_def);

    memset(uart, 0, sizeof(*uart));
    uart->Instance = instance;
    uart->Init.BaudRate = baud;
    uart->Init.WordLength = UART_WORDLENGTH_8B;
    uart->Init.StopBits = UART_STOPBITS_1;
    uart->Init.Parity = UART_PARITY_NONE;
    uart->Init.Mode = UART_MODE_TX_RX;
    uart->Init.HwFlowCtl = UART_HWCONTROL_NONE;
    uart->Init.OverSampling = UART_OVERSAMPLING_16;

    if (HAL_UART_Init(uart) != HAL_OK)
    {
        ASSERT(0);
    }

    /* Peripheral interrupt init*/
    const IRQn_Type irq_num = get_irq_num(id);
    // Note :- priority should be >= configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY
    HAL_NVIC_SetPriority(irq_num, 5, 0);
    HAL_NVIC_EnableIRQ(irq_num);

    __HAL_UART_ENABLE_IT(uart, UART_IT_RXNE);

    return uart;
}

static void MX_UART_Deinit(panglos::UART::Id id)
{
    /* Peripheral interrupt init*/
    const IRQn_Type irq_num = get_irq_num(id);
    HAL_NVIC_DisableIRQ(irq_num);

    HAL_UART_DeInit(get_uart(id));
}

    /*
     *
     */

class ArmUart : public UART
{
    Id id;

    UART_HandleTypeDef *handle;
    RingBuffer<uint8_t> *buffer;
    uint32_t error;

    virtual uint32_t get_error() { uint32_t e = error; error = 0; return e; }

public:
    ArmUart(Id _id, UART_HandleTypeDef *_handle, RingBuffer<uint8_t> *b)
    : id(_id), handle(_handle), buffer(b)
    {   
        ASSERT(buffer);
        mutex = Mutex::create();
        uarts[id] = this;
    }

    ~ArmUart()
    {
        MX_UART_Deinit(id);
        uarts[id] = 0;
    }

    // Implement Output class
    virtual int _putc(char c)
    {
        return send(& c, 1);
    }

    // Implement UART class
    virtual int send(const char* data, int n);

    void irq()
    {
        ASSERT(buffer);

        static const uint32_t err_mask = UART_FLAG_ORE | UART_FLAG_NE | UART_FLAG_FE | UART_FLAG_PE;
        const uint32_t sr = handle->Instance->SR;

        const uint32_t err = sr & err_mask;

        if (err)
        {
            set_error(err);
            __HAL_UART_CLEAR_FLAG(handle, err);
        }

        if (sr & UART_FLAG_RXNE)
        {
            const uint8_t data = handle->Instance->DR;
            /* Clear RXNE interrupt flag */
            //__HAL_UART_SEND_REQ(handle, UART_RXDATA_FLUSH_REQUEST);
            buffer->add(data);
        }
    }

    void set_error(uint32_t e) { error |= e; }
};

    /*
     *
     */

int ArmUart::send(const char* data, int n)
{
    if (!handle)
    {
        return 0;
    }

    Lock lock(mutex);

    HAL_UART_Transmit(handle, (uint8_t*) data, n, 100);
    return n;
}

    /*
     *
     */

UART *UART::create(UART::Id id, int baud, RingBuffer<uint8_t> *b)
{
    UART_HandleTypeDef *uart = MX_UART_Init(id, baud);
    return new ArmUart(id, uart, b);
}

    /*
     *
     */

static void uart_rx_irq(ArmUart *uart)
{
    if (!uart)
    {
        // UART not found!
        return;
    }

    uart->irq();
}

}   //  namespace panglos

    /*
     *
     */

using namespace panglos;

extern "C" void USART1_IRQHandler(void)
{
    uart_rx_irq(uarts[0]);
}

extern "C" void USART2_IRQHandler(void)
{
    uart_rx_irq(uarts[1]);
}

extern "C" void USART3_IRQHandler(void)
{
    uart_rx_irq(uarts[2]);
}

//  FIN
