
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#include "../panglos/stm32/hal.h"

#include <stm32f1xx_hal_dma.h>

#include "../panglos/debug.h"

#include "../panglos/gpio.h"
#include "../panglos/mutex.h"
#include "../panglos/list.h"
#include "../panglos/uart.h"

// TODO : remove me?
#include "../panglos/dispatch.h"
#include "../panglos/list.h"
extern panglos::Dispatch dispatch;

namespace panglos {

class Callback;
extern List<Callback*> callbacks;
static Callback *cb_next;

static Mutex *cb_mutex = Mutex::create_critical_section();

class Callback : public Dispatch::Callback
{
    UART::Buffer *uart;
public:
    uint8_t buff[16];
    int in;
    Callback(UART::Buffer *b) 
    :   Dispatch::Callback(0), uart(b), in(0)
    {
        // TODO : this should be set by the DMA Rx code
        in = sizeof(buff);
    }

    virtual void execute()
    { 
        ASSERT(uart);
        uart->add(buff, in);
        //PO_DEBUG("%s", buff);
        callbacks.push(this, cb_mutex);
    }
};

#define NEXT_FN (Callback **(*)(Callback*)) Dispatch::Callback::next_fn
List<Callback*> callbacks(NEXT_FN);

static UART_HandleTypeDef uart1;
static UART_HandleTypeDef uart2;
static UART_HandleTypeDef uart3;

class ArmUart;

static ArmUart *uarts[3];

class ArmUart : public UART
{
public:
    Id id;

    UART_HandleTypeDef *handle;
    Buffer *buffer;
    uint32_t error;

    virtual uint32_t get_error() { uint32_t e = error; error = 0; return e; }

public:
    ArmUart(Id _id, UART_HandleTypeDef *_handle, Buffer *b);

    ~ArmUart();

    // Implement Output class
    virtual int _putc(char c);

    // Implement UART class
    virtual int send(const char* data, int n);

    int on_rx(const uint8_t *data, int size);

    void irq();

    void set_error(uint32_t e) { error |= e; }
};

    /*
     *  DMA
     */

static DMA_HandleTypeDef dma1_chan5_rx;

    /*
     *
     */

static void dma_init(UART_HandleTypeDef *uart, DMA_HandleTypeDef *dma)
{
    // DMA init

    // enable dma interface ck
    __HAL_RCC_DMA1_CLK_ENABLE();

    // configure handle with required rx/tx params
    dma->Instance = DMA1_Channel5;

    dma->Init.Direction = DMA_PERIPH_TO_MEMORY;
    dma->Init.PeriphInc = DMA_PINC_DISABLE;
    dma->Init.MemInc = DMA_MINC_ENABLE;
    dma->Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
    dma->Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
    dma->Init.Mode = DMA_NORMAL; // or DMA_CIRCULAR
    dma->Init.Priority = DMA_PRIORITY_HIGH;

    // configure the dma tx/rx channel
    HAL_StatusTypeDef status = HAL_DMA_Init(dma);
    ASSERT(status == HAL_OK);

    // associate dma handle with the tx/rx handle
    __HAL_LINKDMA(uart, hdmarx, (*dma));

    // configure priority and enable NVIC for xfer complete irq
    HAL_NVIC_SetPriority(DMA1_Channel5_IRQn, 5, 0);
    HAL_NVIC_EnableIRQ(DMA1_Channel5_IRQn);
}

    /*
     *
     */

static void dma_start(UART_HandleTypeDef *uart, DMA_HandleTypeDef *dma)
{
    // only the first Rx irq is required to start the xfer
    __HAL_UART_DISABLE_IT(uart, UART_IT_RXNE);

    ASSERT(cb_next == 0);
    cb_next = callbacks.pop(cb_mutex);
    HAL_StatusTypeDef status = HAL_UART_Receive_DMA(uart, cb_next->buff, sizeof(cb_next->buff));
    ASSERT(status == HAL_OK);
}

#if 0
extern "C" void HAL_UART_RxHalfCpltCallback(UART_HandleTypeDef *huart)
{
    //p5->set(true);
    ASSERT(huart == & uart1);

    // only the first Rx irq is required to start the xfer
    __HAL_UART_DISABLE_IT(huart, UART_IT_RXNE);
    // Can get called when a line-break or other error occurs
}
#endif

extern "C" void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    ASSERT(huart == & uart1);

    // TODO : how many chars have been xferred?

    Callback *cb = cb_next;
    cb_next = 0;

    dma_start(huart, & dma1_chan5_rx);

    if (cb)
    {
        dispatch.put(cb);
    }
}

extern "C" void DMA1_Channel5_IRQHandler(void)
{
    HAL_DMA_IRQHandler(& dma1_chan5_rx);
}
    
    /*
     *
     */

static UART_HandleTypeDef *get_uart(UART::Id id)
{
    switch (id)
    {
        case UART::UART_1 : return & uart1;
        case UART::UART_2 : return & uart2;
        case UART::UART_3 : return & uart3;
        default : ASSERT(0);
    }

    return 0;
}

static IRQn_Type get_irq_num(UART::Id id)
{
    switch (id)
    {
        case UART::UART_1 : return USART1_IRQn;
        case UART::UART_2 : return USART2_IRQn;
        case UART::UART_3 : return USART3_IRQn;
        default : ASSERT(0);
    }

    return (IRQn_Type) 0;
}

    /*
     *
     */

#if defined(STM32F4xx)

static void init_uart1()
{
    __USART1_CLK_ENABLE();

    GPIO_InitTypeDef gpio_def;

    gpio_def.Pin = GPIO_PIN_9;
    gpio_def.Mode = GPIO_MODE_AF_PP;
    gpio_def.Alternate = GPIO_AF7_USART1;
    gpio_def.Speed = GPIO_SPEED_HIGH;
    gpio_def.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, & gpio_def);

    gpio_def.Pin = GPIO_PIN_10;
    gpio_def.Mode = GPIO_MODE_AF_PP;
    HAL_GPIO_Init(GPIOA, & gpio_def);
}

static void init_uart2()
{
    __USART2_CLK_ENABLE();
        
    GPIO_InitTypeDef gpio_def;

    gpio_def.Pin = GPIO_PIN_2;
    gpio_def.Mode = GPIO_MODE_AF_PP;
    gpio_def.Alternate = GPIO_AF7_USART2;
    gpio_def.Speed = GPIO_SPEED_HIGH;
    gpio_def.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, & gpio_def);

    gpio_def.Pin = GPIO_PIN_3;
    gpio_def.Mode = GPIO_MODE_AF_PP;
    HAL_GPIO_Init(GPIOA, & gpio_def);
}

static void init_uart3()
{
    __USART3_CLK_ENABLE();

    GPIO_InitTypeDef gpio_def;

    gpio_def.Pin = GPIO_PIN_10 | GPIO_PIN_11;
    gpio_def.Mode = GPIO_MODE_AF_PP;
    gpio_def.Alternate = GPIO_AF7_USART3;
    gpio_def.Speed = GPIO_SPEED_HIGH;
    gpio_def.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOB, & gpio_def);
}

#endif // STM32F4xx

    /*
     *
     */

#if defined(STM32F1xx)

// See STM32F103xx Reference Manual :
// AF remap and debug I/O configuration register (AFIO_MAPR)

enum Map {
    MAP_NONE,
    MAP_PARTIAL,
    MAP_FULL
};

static void init_uart1(Map map=MAP_NONE)
{
    //0: No remap (TX/PA9, RX/PA10)
    //1: Remap    (TX/PB6, RX/PB7)

    __HAL_RCC_USART1_CLK_ENABLE();

    switch (map)
    {
        case MAP_NONE:
        {
            static const PortPin rx = { GPIOA, GPIO_PIN_10 };
            static const PortPin tx = { GPIOA, GPIO_PIN_9 };
            __HAL_AFIO_REMAP_USART1_DISABLE();
            INIT_AF_GPIO(& rx, GPIO_MODE_AF_INPUT);
            INIT_AF_GPIO(& tx, GPIO_MODE_AF_PP);
            break;
        }
        case MAP_FULL:
        {
            static const PortPin rx = { GPIOB, GPIO_PIN_7 };
            static const PortPin tx = { GPIOB, GPIO_PIN_6 };
            __HAL_AFIO_REMAP_USART1_ENABLE();
            INIT_AF_GPIO(& rx, GPIO_MODE_AF_INPUT);
            INIT_AF_GPIO(& tx, GPIO_MODE_AF_PP);
            break;
        }
        default :
        {
            ASSERT(0);
        }
    }
}


static void init_uart2(Map map=MAP_NONE)
{
    //0: No remap (CTS/PA0, RTS/PA1, TX/PA2, RX/PA3, CK/PA4)
    //1: Remap    (CTS/PD3, RTS/PD4, TX/PD5, RX/PD6, CK/PD7

    __HAL_RCC_USART2_CLK_ENABLE();

    switch (map)
    {
        case MAP_NONE:
        {
            static const PortPin rx = { GPIOA, GPIO_PIN_3 };
            static const PortPin tx = { GPIOA, GPIO_PIN_2 };
            __HAL_AFIO_REMAP_USART2_DISABLE();
            INIT_AF_GPIO(& rx, GPIO_MODE_AF_INPUT);
            INIT_AF_GPIO(& tx, GPIO_MODE_AF_PP);
            break;
        }
        case MAP_FULL:
        {
            static const PortPin rx = { GPIOD, GPIO_PIN_6 };
            static const PortPin tx = { GPIOD, GPIO_PIN_5 };
            __HAL_AFIO_REMAP_USART2_ENABLE();
            INIT_AF_GPIO(& rx, GPIO_MODE_AF_INPUT);
            INIT_AF_GPIO(& tx, GPIO_MODE_AF_PP);
            break;
        }
        default :
        {
            ASSERT(0);
        }
    }
}

static void init_uart3(Map map=MAP_NONE)
{
    // 00: No remap      (TX/PB10, RX/PB11, CK/PB12, CTS/PB13, RTS/PB14)
    // 01: Partial remap (TX/PC10, RX/PC11, CK/PC12, CTS/PB13, RTS/PB14)
    // 11: Full remap    (TX/PD8,  RX/PD9,  CK/PD10, CTS/PD11, RTS/PD12)

    __HAL_RCC_USART3_CLK_ENABLE();

    switch (map)
    {
        case MAP_NONE:
        {
            static const PortPin rx = { GPIOB, GPIO_PIN_11 };
            static const PortPin tx = { GPIOB, GPIO_PIN_10 };
            __HAL_AFIO_REMAP_USART3_DISABLE();
            INIT_AF_GPIO(& rx, GPIO_MODE_AF_INPUT);
            INIT_AF_GPIO(& tx, GPIO_MODE_AF_PP);
            break;
        }
        case MAP_PARTIAL:
        {
            static const PortPin rx = { GPIOC, GPIO_PIN_11 };
            static const PortPin tx = { GPIOC, GPIO_PIN_10 };
            __HAL_AFIO_REMAP_USART3_PARTIAL();
            INIT_AF_GPIO(& rx, GPIO_MODE_AF_INPUT);
            INIT_AF_GPIO(& tx, GPIO_MODE_AF_PP);
            break;
        }
        case MAP_FULL:
        {
            static const PortPin rx = { GPIOD, GPIO_PIN_9 };
            static const PortPin tx = { GPIOD, GPIO_PIN_8 };
            __HAL_AFIO_REMAP_USART3_ENABLE();
            INIT_AF_GPIO(& rx, GPIO_MODE_AF_INPUT);
            INIT_AF_GPIO(& tx, GPIO_MODE_AF_PP);
            break;
        }
        default :
        {
            ASSERT(0);
        }
    }
}

#endif // STM32F1xx

    /*
     *
     */

static UART_HandleTypeDef* MX_UART_Init(panglos::UART::Id id, uint32_t baud, int irq_level)
{

    HAL_StatusTypeDef status;
    UART_HandleTypeDef *uart = get_uart(id);
    USART_TypeDef* instance = 0;

    switch (id)
    {
        case UART::UART_1 :
        {
            instance = USART1;
            init_uart1();
            break;
        }
        case UART::UART_2 :
        {
            instance = USART2;
            init_uart2();
            break;
        }
        case UART::UART_3 :
        {
            instance = USART3;
            init_uart3();
            break;
        }
        default :
        {
            ASSERT(0);
        }
    }

    memset(uart, 0, sizeof(*uart));
    uart->Instance = instance;
    uart->Init.BaudRate = baud;
    uart->Init.WordLength = UART_WORDLENGTH_8B;
    uart->Init.StopBits = UART_STOPBITS_1;
    uart->Init.Parity = UART_PARITY_NONE;
    uart->Init.Mode = UART_MODE_TX_RX;
    uart->Init.HwFlowCtl = UART_HWCONTROL_NONE;
    uart->Init.OverSampling = UART_OVERSAMPLING_16;

    status = HAL_UART_Init(uart);
    ASSERT(status == HAL_OK);

    /* Peripheral interrupt init*/
    const IRQn_Type irq_num = get_irq_num(id);
    // Note :- priority should be >= configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY
    HAL_NVIC_SetPriority(irq_num, irq_level, 0);
    HAL_NVIC_EnableIRQ(irq_num);

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

ArmUart::ArmUart(Id _id, UART_HandleTypeDef *_handle, Buffer *b)
: id(_id), handle(_handle), buffer(b), error(0)
{   
    ASSERT(buffer);
    uarts[id] = this;
}

ArmUart::~ArmUart()
{
    MX_UART_Deinit(id);
    uarts[id] = 0;
}

// Implement Output class
int ArmUart::_putc(char c)
{
    return send(& c, 1);
}

int ArmUart::on_rx(const uint8_t *data, int size)
{
    return buffer->add(data, size);
}

void ArmUart::irq()
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
        uint8_t data = handle->Instance->DR;
        /* Clear RXNE interrupt flag */
        //__HAL_UART_SEND_REQ(handle, UART_RXDATA_FLUSH_REQUEST);
        on_rx(& data, 1);
    }
}

    /*
     *
     */

int ArmUart::send(const char* data, int n)
{
    if (!handle)
    {
        return 0;
    }

    // TODO : where is this global mutex!
    Lock lock(mutex);

    HAL_UART_Transmit(handle, (uint8_t*) data, n, 100);
    return n;
}

    /*
     *
     */

UART *UART::create(UART::Id id, int baud, Buffer *b, int irq_level)
{
    UART_HandleTypeDef *uart = MX_UART_Init(id, baud, irq_level);
    ArmUart * arm_uart = new ArmUart(id, uart, b);

    if (id == UART::UART_1)
    {
        if (callbacks.empty())
        {
            for (int i = 0; i < 20; i++)
            {
                Callback *cb = new Callback(arm_uart->buffer);
                callbacks.push(cb, cb_mutex);
            }
        }

        dma_init(uart, & dma1_chan5_rx);
        dma_start(uart, & dma1_chan5_rx);
    }

    if (id != UART::UART_1)
    {
        __HAL_UART_ENABLE_IT(uart, UART_IT_RXNE);
    }
    return arm_uart;
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
    //uart_rx_irq(uarts[0]);
    HAL_UART_IRQHandler(& uart1);
    //dma_start(& uart1, & dma1_chan5_rx, 0);
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
