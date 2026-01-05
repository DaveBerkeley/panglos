
#include "panglos/debug.h"
#include "panglos/stm32/uart_f1.h"

//#include "uart.h"

class _UART;

static _UART *uarts[3];

typedef struct {
    uint32_t pwr_mask;
    volatile uint32_t *pwr_reg;
    IRQn_Type irq;
}   UartInfo;

static const UartInfo *get_info(USART_TypeDef *base)
{
    switch ((uint32_t) base)
    {
        case USART1_BASE : 
        {
            const static UartInfo info = { RCC_APB2ENR_USART1EN, & RCC->APB2ENR, USART1_IRQn };
            return & info;
        }
        case USART2_BASE : 
        {
            const static UartInfo info = { RCC_APB1ENR_USART2EN, & RCC->APB1ENR, USART2_IRQn };
            return & info;
        }
        case USART3_BASE : 
        {
            const static UartInfo info = { RCC_APB1ENR_USART3EN, & RCC->APB1ENR, USART3_IRQn };
            return & info;
        }
    }
    return 0;
}

    /*
     *
     */

class _UART : public STM32F1_UART
{
    USART_TypeDef *base;
    static uint32_t ck_enabled;
    char *rx_buff;
    uint32_t rx_size;
    uint32_t rx_wr;
    uint32_t rx_rd;
    uint32_t rx_mask;

    void enable_port_power()
    {
        const UartInfo *info = get_info(base);
        ASSERT(info);

        if (!(ck_enabled & info->pwr_mask))
        {
            *info->pwr_reg |= info->pwr_mask;
            ck_enabled |= info->pwr_mask;
        }
    }

    void make_buffer()
    {
        if (!rx_size) return;
        rx_buff = new char[rx_size];
        rx_wr = 0;
        rx_rd = 0;
        rx_mask = rx_size - 1;

        // rx_buff must be a power of 2
        int bits = 0;
        for (uint32_t s = rx_size; s; s >>= 1)
        {
            if (s & 0x01) bits += 1;
        }
        ASSERT(bits == 1);
    }

    void buff_write(char c)
    {
        const uint32_t wr = (rx_wr + 1);
        if (wr == rx_rd) return; // buffer full
        rx_buff[rx_mask & (rx_wr++)] = c;
    }

public:
    _UART(USART_TypeDef *_base, uint32_t _rx_size)
    :   base(_base),
        rx_buff(0),
        rx_size(_rx_size)
    {
        enable_port_power();
        make_buffer();
 
        // Configure USART1
        // Assuming 8MHz clock, baud rate = 115200
        // BRR = 8000000 / 115200 = 69.44 ≈ 0x45
        base->BRR = 0x45;  // For 115200 baud @ 8MHz
 
        // Enable USART, TX
        base->CR1 = USART_CR1_TE | USART_CR1_UE;

        // Enable RX interrupts
        if (rx_size)
            base->CR1 |= USART_CR1_RXNEIE | USART_CR1_RE;

        switch ((uint32_t) base)
        {
            case USART1_BASE : uarts[0] = this; break;
            case USART2_BASE : uarts[1] = this; break;
            case USART3_BASE : uarts[2] = this; break;
        }

        const UartInfo *info = get_info(base);
        ASSERT(info);
        NVIC_EnableIRQ(info->irq);
        NVIC_SetPriority(info->irq, 0);
    }

    ~_UART()
    {
        delete[] rx_buff;
    }

    // Send a single character
    void putchar(char c)
    {
        // Wait until TX buffer is empty
        while(!(base->SR & USART_SR_TXE))
            ;
        base->DR = c;
    }

    virtual int tx(const char* data, int n)
    {
        for (int i = 0; i < n; i++)
        {
            putchar(*data++);
        }
        return n;
    }

    virtual int rx(char* data, int n) override
    {
        for (int i = 0; i < n; i++)
        {
            if (rx_wr == rx_rd) return i;
            const char c = rx_buff[rx_mask & (rx_rd++)];
            *data++ = c;
        }
        return n;
    }

    void on_interrupt()
    {
        // check if it is an Rx interrupt
        if (base->SR & USART_SR_RXNE)
        {
            char c = base->DR;
            buff_write(c);
            // clear the interrupt
            base->SR &= ~USART_SR_RXNE;
        }
    }

    static void on_interrupt(uint32_t idx)
    {
        _UART *uart = uarts[idx];
        ASSERT(uart);
        uart->on_interrupt();
    }
};

uint32_t _UART::ck_enabled = 0;

panglos::UART *STM32F1_UART::create(USART_TypeDef *base, uint32_t rx_size)
{
    return new _UART(base, rx_size);
}

extern "C" void USART1_IRQHandler(void)
{
    _UART::on_interrupt(0);
}

extern "C" void USART2_IRQHandler(void)
{
    _UART::on_interrupt(1);
}

extern "C" void USART3_IRQHandler(void)
{
    _UART::on_interrupt(2);
}

//  FIN
