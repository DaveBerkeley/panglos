
#include "../panglos/stm32/stm32fxxx_hal.h"

#include "../panglos/debug.h"
#include "../panglos/spi.h"
#include "../panglos/stm32/hal.h"

namespace panglos {

    /**
    * @brief SPI Initialization Function
    * @param handle
    * @param instance
    * @retval None
    */

static void MX_SPI_Init(SPI_HandleTypeDef *handle, SPI_TypeDef *instance)
{
    /* SPI1 parameter configuration*/
    handle->Instance = instance;
    handle->Init.Mode = SPI_MODE_MASTER;
    handle->Init.Direction = SPI_DIRECTION_2LINES;
    handle->Init.DataSize = SPI_DATASIZE_8BIT;
    handle->Init.CLKPolarity = SPI_POLARITY_LOW;
    handle->Init.CLKPhase = SPI_PHASE_1EDGE;
    handle->Init.NSS = SPI_NSS_SOFT;
    handle->Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_2;
    handle->Init.FirstBit = SPI_FIRSTBIT_MSB;
    handle->Init.TIMode = SPI_TIMODE_DISABLE;
    handle->Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
    handle->Init.CRCPolynomial = 10;
    const HAL_StatusTypeDef err = HAL_SPI_Init(handle);
    ASSERT(err == HAL_OK);
}

    /*
     *
     */

class ArmSpi : public SPI
{
public:
    SPI_HandleTypeDef handle;

    ArmSpi(SPI_TypeDef *instance, Mutex *mutex)
    : SPI(mutex)
    {
        MX_SPI_Init(& handle, instance);
    }

    virtual bool write(const uint8_t *data, int size)
    {
        HAL_SPI_Transmit(& handle, (uint8_t *) data, size, HAL_MAX_DELAY);
        return true;
    }

    virtual bool read(const uint8_t *data, uint8_t *rd, int size)
    {
        HAL_SPI_TransmitReceive(& handle, (uint8_t*) data, rd, size, HAL_MAX_DELAY);
        return true;
    }
};

    /*
     *
     */

#if defined(STM32F4xx)
static void spi1_init()
{
    __HAL_RCC_SPI1_CLK_ENABLE();

    GPIO_InitTypeDef  gpio_def;
    gpio_def.Pin       = GPIO_PIN_5 | GPIO_PIN_6 | GPIO_PIN_7;
    gpio_def.Mode      = GPIO_MODE_AF_PP;
    gpio_def.Pull      = GPIO_PULLUP;
    gpio_def.Speed     = GPIO_SPEED_FREQ_HIGH;
    gpio_def.Alternate = GPIO_AF5_SPI1;
    HAL_GPIO_Init(GPIOA, & gpio_def);
}

static void spi2_init()
{
    __HAL_RCC_SPI2_CLK_ENABLE();

    GPIO_InitTypeDef  gpio_def;
    gpio_def.Pin       = GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_15;
    gpio_def.Mode      = GPIO_MODE_AF_PP;
    gpio_def.Pull      = GPIO_PULLUP;
    gpio_def.Speed     = GPIO_SPEED_FREQ_HIGH;
    gpio_def.Alternate = GPIO_AF5_SPI2;
    HAL_GPIO_Init(GPIOB, & gpio_def);
}
#endif

#if defined(STM32F1xx)
static void spi1_init(bool remap=false)
{
    // AF remap and debug I/O configuration register (AFIO_MAPR)
    // 0: No remap (NSS/PA4, SCK/PA5, MISO/PA6, MOSI/PA7)
    // 1: Remap (NSS/PA15, SCK/PB3, MISO/PB4, MOSI/PB5)

    static const PortPin no_map[] = {
        //{   GPIOA, GPIO_PIN_4 },
        {   GPIOA, GPIO_PIN_5 | GPIO_PIN_6 | GPIO_PIN_7 },
        { 0, 0 },
    };
    static const PortPin do_remap[] = {
        //{   GPIOA, GPIO_PIN_15 },
        {   GPIOB, GPIO_PIN_3 | GPIO_PIN_4 | GPIO_PIN_5 },
        { 0, 0 },
    };

    __HAL_RCC_SPI1_CLK_ENABLE();

    if (remap)
    {
        __HAL_AFIO_REMAP_SPI1_ENABLE();
    }
    else
    {
        __HAL_AFIO_REMAP_SPI1_DISABLE();
    }

    INIT_AF_GPIOs(remap ? do_remap : no_map, GPIO_MODE_AF_PP);
}

static void spi2_init()
{
    ASSERT(0);
#if 0
    GPIO_InitTypeDef  gpio_def;
    gpio_def.Pin       = GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_15;
    gpio_def.Mode      = GPIO_MODE_AF_PP;
    gpio_def.Pull      = GPIO_PULLUP;
    gpio_def.Speed     = GPIO_SPEED_FREQ_HIGH;
    __HAL_AFIO_REMAP_SPI2_ENABLE();
    HAL_GPIO_Init(GPIOB, & gpio_def);
#endif
}
#endif // STMF1xx

SPI *SPI::create(ID _id, Mutex *mutex)
{
    switch (_id)
    {
        case SPI_1 :
        {
            spi1_init();
            return new ArmSpi(SPI1, mutex);
        }
        case SPI_2 :
        {
            spi2_init();
            return new ArmSpi(SPI2, mutex);
        }
        default :
        {
            ASSERT(0);
        }
    }

    return 0;
}

}   //  namespace panglos

//  FIN
