
#include <string.h>

#if defined(STM32F4)

#include "panglos/debug.h"
#undef UNUSED

#include "panglos/stm32/uart.h"
#include "panglos/stm32/gpio_arm.h"
#include "panglos/stm32/lut.h"
#include "panglos/stm32/irq.h"

    /*
     *
     */

static void uart_enable_clock(USART_TypeDef *uart)
{
    // stm32f446 : USART1 USART2 USART3 UART4 UART5 USART6
    ASSERT(uart);
#if defined(USART1)
    if (uart == USART1) { __HAL_RCC_USART1_CLK_ENABLE(); return; }
#endif
#if defined(USART2)
    if (uart == USART2) { __HAL_RCC_USART2_CLK_ENABLE(); return; }
#endif
#if defined(USART3)
    if (uart == USART3) { __HAL_RCC_USART3_CLK_ENABLE(); return; }
#endif
#if defined(UART4)
    if (uart == UART4) { __HAL_RCC_UART4_CLK_ENABLE(); return; }
#endif
#if defined(UART5)
    if (uart == UART5) { __HAL_RCC_UART5_CLK_ENABLE(); return; }
#endif
#if defined(UART6)
    if (uart == UART6) { __HAL_RCC_UART6_CLK_ENABLE(); return; }
#endif
    ASSERT(0);
}

    /*
     *
     */

IRQn_Type panglos::UartConfig::get_irq_type(USART_TypeDef *uart)
{
    ASSERT(uart);
#if defined(USART1)
    if (uart == USART1) return USART1_IRQn;
#endif
#if defined(USART2)
    if (uart == USART2) return USART3_IRQn;
#endif
#if defined(USART3)
    if (uart == USART3) return USART3_IRQn;
#endif
#if defined(UART4)
    if (uart == UART4) return UART4_IRQn;
#endif
#if defined(UART5)
    if (uart == UART5) return UART5_IRQn;
#endif
#if defined(UART6)
    if (uart == UART6) return UART6_IRQn;
#endif
    ASSERT(0);
    return IRQn_Type(0);
}

    /*
     *
     */

namespace panglos {

class STM32_UART : public panglos::UART
{
    UART_HandleTypeDef handle;
    DMA_HandleTypeDef dma_rx;
    DMA_HandleTypeDef dma_tx;

    class DmaTxHandler : public IrqHandler
    {
        STM32_UART *uart;
    public:
        DmaTxHandler(STM32_UART *_uart) : uart(_uart) { }
        virtual void run() override { uart->on_tx_dma(); }
    };

    class DmaRxHandler : public IrqHandler
    {
        STM32_UART *uart;
    public:
        DmaRxHandler(STM32_UART *_uart) : uart(_uart) { }
        virtual void run() override { uart->on_rx_dma(); }
    };

    class RxHandler : public IrqHandler
    {
        STM32_UART *uart;
    public:
        RxHandler(STM32_UART *_uart) : uart(_uart) { }
        virtual void run() override { uart->on_rx_interrupt(); }
    };

    struct DmaTxHandler dma_rx_irq;
    struct DmaTxHandler dma_tx_irq;
    struct DmaTxHandler rx_irq;

        /*
         *
         */

    void on_rx_interrupt()
    {
        ASSERT(0);
        HAL_UART_IRQHandler(& handle);
    }

    void on_rx_dma()
    {
        HAL_DMA_IRQHandler(& dma_rx);
    }

    void on_tx_dma()
    {
        HAL_DMA_IRQHandler(& dma_tx);
    }

private:
    void enable_irq(IRQn_Type irq, uint32_t preempt_level, IrqHandler *handler)
    {
        HAL_NVIC_SetPriority(irq, preempt_level, 0);
        set_vector(irq, handler);
        HAL_NVIC_EnableIRQ(irq);
    }

    bool init_uart(const struct UartConfig *config)
    {
        uint32_t mode = 0;
        if (config->rx.port)
            mode |= UART_MODE_RX;
        if (config->tx.port)
            mode |= UART_MODE_TX;

        if (!mode) return false;

        handle.Instance = config->uart;  // Specify which USART peripheral to use
        handle.Init.BaudRate = config->baud; // Set baud rate
        handle.Init.WordLength = UART_WORDLENGTH_8B; // 8 data bits
        handle.Init.StopBits = UART_STOPBITS_1;      // 1 stop bit
        handle.Init.Parity = UART_PARITY_NONE;       // No parity
        handle.Init.Mode = mode;
        handle.Init.HwFlowCtl = UART_HWCONTROL_NONE; // No hardware flow control
        handle.Init.OverSampling = UART_OVERSAMPLING_16; // Oversampling setting [citation:5][citation:11]

        // Initialize the UART with the configured parameters
        HAL_StatusTypeDef ok = HAL_UART_Init(& handle);
        if (ok != HAL_OK) !hal_error(ok);

        enable_irq(panglos::UartConfig::get_irq_type(config->uart), 5, & rx_irq);

        return true;        
    }

    void init_gpio(const struct Pin *pin)
    {
        gpio_enable_clock(pin->port);
        gpio_alloc(pin->port, pin->pin);

        GPIO_InitTypeDef config = {0};

        config.Pin = pin->pin;
        config.Mode = pin->mode;
        config.Pull = GPIO_NOPULL ; // GPIO_PULLUP;
        config.Speed = GPIO_SPEED_FREQ_HIGH;
        config.Alternate = pin->alt;
        HAL_GPIO_Init(pin->port, & config);
    }

    bool init_dma(const struct UartConfig *config, bool rx)
    {
        DMA_HandleTypeDef *dma_handle = 0;
        const struct Dma::Config *dma_config = 0;
        uint32_t direction = 0;
        struct IrqHandler *handler = 0;

        if (rx)
        {
            dma_handle = & dma_rx;
            dma_config =  & config->rx_dma;
            direction = DMA_PERIPH_TO_MEMORY;
            handler = & dma_rx_irq;
        }
        else
        {
            dma_handle = & dma_tx;
            dma_config =  & config->tx_dma;
            direction = DMA_MEMORY_TO_PERIPH;
            handler = & dma_tx_irq;
        }

        if (!dma_config->dma) return true;

        memset(dma_handle, 0, sizeof(*dma_handle));

        dma_handle->Instance = dma_config->stream;
        dma_handle->Init.Channel = dma_config->channel;
        dma_handle->Init.Direction = direction;
        dma_handle->Init.PeriphInc = DMA_PINC_DISABLE;
        dma_handle->Init.MemInc = DMA_MINC_ENABLE;
        dma_handle->Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
        dma_handle->Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
        dma_handle->Init.Mode = DMA_NORMAL;
        dma_handle->Init.Priority = DMA_PRIORITY_LOW;
        dma_handle->Init.FIFOMode = DMA_FIFOMODE_DISABLE;

        HAL_StatusTypeDef err = HAL_DMA_Init(dma_handle);
        if (err != HAL_OK) return !hal_error(err);

        if (rx)
        {
            __HAL_LINKDMA((& handle), hdmarx, *dma_handle);
        }
        else
        {
            __HAL_LINKDMA((& handle), hdmatx, *dma_handle);
        }

        enable_irq(Dma::get_irq_type(dma_config->stream), 5, handler);

        return true;
    }

public:
    STM32_UART()
    :   dma_rx_irq(this),
        dma_tx_irq(this),
        rx_irq(this)
    {
        memset(& handle, 0, sizeof(handle));
    }

    bool init(const struct UartConfig *config)
    {
        uart_enable_clock(config->uart);
        init_gpio(& config->rx);
        init_gpio(& config->tx);

        init_dma(config, true);     // rx
        init_dma(config, false);    // tx

        return init_uart(config);
    }

    // Implement UART
    virtual int rx(char* data, int n) override
    {
        //HAL_StatusTypeDef ok = HAL_UARTEx_ReceiveToIdle_DMA(& handle, (uint8_t*) data, n);
        //HAL_StatusTypeDef ok = HAL_UART_Receive_DMA(& handle, (uint8_t*) data, n);
        HAL_StatusTypeDef ok = HAL_UART_Receive_IT(& handle, (uint8_t*) data, n);
        if (ok != HAL_OK)
        {
            hal_error(ok);
        }
        return (ok == HAL_OK) ? n : 0;
    }

    virtual int tx(const char* data, int n) override
    {
        // TODO : using interrupts or DMA if more than n chars ??
        HAL_StatusTypeDef ok = HAL_UART_Transmit(& handle, (const uint8_t*) data, n, HAL_MAX_DELAY);
        //HAL_StatusTypeDef ok = HAL_UART_Transmit_DMA(& handle, (const uint8_t*) data, n);
        if (ok != HAL_OK)
        {
            hal_error(ok);
        }
        return (ok == HAL_OK) ? n : 0;
    } 

    static void on_rx_event(UART_HandleTypeDef *huart, uint16_t Size)
    {
        ASSERT(0);
    }

    static void on_error_event(UART_HandleTypeDef *huart)
    {
        ASSERT(0);
    }
};

    /*
     *
     */

void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size)
{
    STM32_UART::on_rx_event(huart, Size);
}

void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
    STM32_UART::on_error_event(huart);
}

#endif  //  STM32F4

    /*
     *
     */

UART *create_uart(const UartConfig *config)
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
