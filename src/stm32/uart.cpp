
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#include "../panglos/stm32/stm32fxxx_hal.h"
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

    /*
     *  DMA
     */

#define DISPATCH_BUFF_SIZE 16
#define DISPATCH_BUFFERS 8

class Callback : public Dispatch::Callback
{
    UART::Buffer *uart;
public:
    uint8_t buff[DISPATCH_BUFF_SIZE];
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
        // return this callback to the pool
        list.push(this, mutex);
    }

    typedef List<Callback*> Callbacks;
    static Callbacks list;
    static Callback **next_fn(Callback *cb) { return (Callback**) & cb->next; }
    static Mutex *mutex;
};

Callback::Callbacks Callback::list(Callback::next_fn);

Mutex *Callback::mutex = Mutex::create_critical_section();

    /*
     *
     */

static UART_HandleTypeDef uart1;
static UART_HandleTypeDef uart2;
static UART_HandleTypeDef uart3;

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

    /*
     *
     */

class ArmUart : public UART
{
public:
    Id id;

    UART_HandleTypeDef *handle;
    Buffer *buffer;
    uint32_t error;
    Callback *next_cb;

    virtual uint32_t get_error() { uint32_t e = error; error = 0; return e; }

public:
    ArmUart(Id _id, UART_HandleTypeDef *_handle, Buffer *b);

    ~ArmUart();

    // Implement Output class
    virtual int _putc(char c);
    virtual int _puts(char *s, int n);

    // Implement UART class
    virtual int send(const char* data, int n);

    int on_rx(const uint8_t *data, int size);

    void irq();

    void set_error(uint32_t e) { error |= e; }

    Callback *get_cb()
    {
        if (!next_cb)
        {
            next_cb = Callback::list.pop(Callback::mutex);
        }

        return next_cb;
    }
};

static ArmUart *uarts[3];

    /*
     *
     */

static DMA_HandleTypeDef dma_rx_1;
static DMA_HandleTypeDef dma_tx_1;
static DMA_HandleTypeDef dma_rx_2;
static DMA_HandleTypeDef dma_tx_2;
static DMA_HandleTypeDef dma_rx_3;
static DMA_HandleTypeDef dma_tx_3;

#if defined(STM32F1xx)

static DMA_Channel_TypeDef *get_dma_instance(UART::Id id, bool rx)
{
    // See manual section 13.3.7 on DMA request mapping
    switch (id)
    {
        case UART::UART_1 : return rx ? DMA1_Channel5 : DMA1_Channel4;
        case UART::UART_2 : return rx ? DMA1_Channel6 : DMA1_Channel7;
        case UART::UART_3 : return rx ? DMA1_Channel3 : DMA1_Channel2;
        default : ASSERT(0);
    }
    return 0;
}

static IRQn_Type get_dma_irq(UART::Id id, bool rx)
{
    switch (id)
    {
        case UART::UART_1 : return rx ? DMA1_Channel5_IRQn : DMA1_Channel4_IRQn;
        case UART::UART_2 : return rx ? DMA1_Channel6_IRQn : DMA1_Channel7_IRQn;
        case UART::UART_3 : return rx ? DMA1_Channel3_IRQn : DMA1_Channel2_IRQn;
        default : ASSERT(0);
    }
    return (IRQn_Type) 0;
}

static DMA_HandleTypeDef *get_dma_handle(UART::Id id, bool rx)
{
    switch (id)
    {
        case UART::UART_1 : return rx ? & dma_rx_1 : & dma_tx_1;
        case UART::UART_2 : return rx ? & dma_rx_2 : & dma_tx_2;
        case UART::UART_3 : return rx ? & dma_rx_3 : & dma_tx_3;
        default : ASSERT(0);
    }
    return 0;
}

#endif

static void dma_init(UART::Id id, bool rx, int irq_level)
{
    // DMA Rx init
    UART_HandleTypeDef *uart = get_uart(id);
    DMA_HandleTypeDef *dma = get_dma_handle(id, rx);

    // enable dma interface ck
    __HAL_RCC_DMA1_CLK_ENABLE();

    // configure handle with required rx/tx params
    dma->Instance = get_dma_instance(id, rx);

    dma->Init.Direction = rx ? DMA_PERIPH_TO_MEMORY : DMA_MEMORY_TO_PERIPH;
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
    if (rx)
    {
        __HAL_LINKDMA(uart, hdmarx, (*dma));
    }
    else
    {
        __HAL_LINKDMA(uart, hdmatx, (*dma));
    }

    // configure priority and enable NVIC for xfer complete irq
    HAL_NVIC_SetPriority(get_dma_irq(id, rx), irq_level, 0);
    HAL_NVIC_EnableIRQ(get_dma_irq(id, rx));
}

    /*
     *
     */

static void dma_start_rx(ArmUart *uart, DMA_HandleTypeDef *dma)
{
    Callback *cb = uart->get_cb();
    ASSERT(cb);
    HAL_StatusTypeDef status = HAL_UART_Receive_DMA(uart->handle, cb->buff, sizeof(cb->buff));
    ASSERT(status == HAL_OK);
}

    /*
     *
     */

extern "C" void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    ASSERT(huart == & uart1);

    ArmUart *uart = uarts[0];

    // TODO : how many chars have been xferred?

    Callback *cb = uart->get_cb();
    uart->next_cb = 0;

    dma_start_rx(uart, & dma_rx_1);

    if (cb)
    {
        dispatch.put(cb);
    }
}

extern "C" void DMA1_Channel5_IRQHandler(void)
{
    HAL_DMA_IRQHandler(& dma_rx_1);
}
 
extern "C" void DMA1_Channel4_IRQHandler(void)
{
    ASSERT(0);
    HAL_DMA_IRQHandler(& dma_tx_1);
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
: id(_id), handle(_handle), buffer(b), error(0), next_cb(0)
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

int ArmUart::_puts(char *s, int n)
{
    return send(s, n);
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

    //HAL_UART_Transmit_DMA(handle, (uint8_t*) data, n);
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
        if (Callback::list.empty())
        {
            Lock lock(Callback::mutex);

            for (int i = 0; i < DISPATCH_BUFFERS; i++)
            {
                Callback *cb = new Callback(arm_uart->buffer);
                Callback::list.push(cb, 0);
            }
        }

        dma_init(id, true, irq_level);
        dma_start_rx(arm_uart, get_dma_handle(id, true));
    }
    else
    {
        // Use Rx Not Empty interrupt for the other UARTs
        __HAL_UART_ENABLE_IT(uart, UART_IT_RXNE);
    }

    if (id == UART::UART_3)
    {
        //dma_init(id, false, irq_level);
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

extern "C" void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
    // TODO : called on error. Needs to recover from it, especially during DMA.
    //ASSERT(0);
}

extern "C" void USART1_IRQHandler(void)
{
    HAL_UART_IRQHandler(& uart1);
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
