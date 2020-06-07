
#include <string.h>

#include "panglos/stm32/stm32fxxx_hal.h"
#include <stm32f4xx_hal_i2s.h>

#include <panglos/debug.h>

#include <panglos/gpio.h>
#include <panglos/stm32/gpio_arm.h>
#include <panglos/i2s.h>

namespace panglos {

static IRQn_Type get_irq_num(I2S::ID id)
{
    switch (id)
    {
        case I2S::I2S_1 : return SPI1_IRQn;
        case I2S::I2S_2 : return SPI2_IRQn;
        default : ASSERT(0);
    }

    return (IRQn_Type) 0;
}

static void init_i2s1()
{
    gpio_alloc(GPIOA, GPIO_PIN_4);
    gpio_alloc(GPIOA, GPIO_PIN_5); // shared with LED on F446 board!
    gpio_alloc(GPIOA, GPIO_PIN_7);

    __HAL_RCC_SPI1_CLK_ENABLE();

    GPIO_InitTypeDef  gpio_def;
    gpio_def.Pin       = GPIO_PIN_4 | GPIO_PIN_5 | GPIO_PIN_7;
    gpio_def.Mode      = GPIO_MODE_AF_PP;
    gpio_def.Pull      = GPIO_PULLUP;
    gpio_def.Speed     = GPIO_SPEED_FREQ_HIGH;
    gpio_def.Alternate = GPIO_AF5_SPI1;
    HAL_GPIO_Init(GPIOA, & gpio_def);
}

static void init_i2s2()
{
    gpio_alloc(GPIOB, GPIO_PIN_3);
    gpio_alloc(GPIOB, GPIO_PIN_5);
    gpio_alloc(GPIOA, GPIO_PIN_15);

    __HAL_RCC_SPI1_CLK_ENABLE();

    GPIO_InitTypeDef  gpio_def;
    gpio_def.Pin       = GPIO_PIN_3 | GPIO_PIN_5;
    gpio_def.Mode      = GPIO_MODE_AF_PP;
    gpio_def.Pull      = GPIO_PULLUP;
    gpio_def.Speed     = GPIO_SPEED_FREQ_HIGH;
    gpio_def.Alternate = GPIO_AF5_SPI2;
    HAL_GPIO_Init(GPIOB, & gpio_def);

    gpio_def.Pin       = GPIO_PIN_15;
    HAL_GPIO_Init(GPIOA, & gpio_def);
}

    /*
     *
     */

class Arm_I2S : public I2S
{
    I2S_HandleTypeDef handle;

public:
    Arm_I2S(ID id)
    {
        memset(& handle, 0, sizeof(handle));

        switch (id)
        {
            case I2S_1 :
            {
                init_i2s1();
                break;
            }
            case I2S_2 :
            {
                init_i2s2();
                break;
            }
            default :
            {
                ASSERT_ERROR(0, "Not implemented");
            }
        }

        /* Peripheral interrupt init*/
        const IRQn_Type irq_num = get_irq_num(id);
        // Note :- priority should be >= configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY
        HAL_NVIC_SetPriority(irq_num, 5, 0);
        HAL_NVIC_EnableIRQ(irq_num);

        // TODO : DMA Config

        __HAL_RCC_PLLI2S_ENABLE();

        handle.Instance = SPI1;
        handle.Init.Mode = I2S_MODE_MASTER_RX;
//#define I2S_STANDARD_PHILIPS             (0x00000000U)
//#define I2S_STANDARD_MSB                 (0x00000010U)
//#define I2S_STANDARD_LSB                 (0x00000020U)
//#define I2S_STANDARD_PCM_SHORT           (0x00000030U)
//#define I2S_STANDARD_PCM_LONG            (0x000000B0U)
        handle.Init.Standard = I2S_STANDARD_LSB; // ?????????
        handle.Init.DataFormat = I2S_DATAFORMAT_24B;
        handle.Init.MCLKOutput = I2S_MCLKOUTPUT_ENABLE;
        handle.Init.AudioFreq = I2S_AUDIOFREQ_16K;
        handle.Init.CPOL = I2S_CPOL_HIGH; // ?????????
        handle.Init.ClockSource = I2S_CLOCK_PLL;
        handle.Init.FullDuplexMode = I2S_FULLDUPLEXMODE_DISABLE;

        HAL_StatusTypeDef okay;
        okay = HAL_I2S_Init(& handle);
        ASSERT(okay == HAL_OK);
    }

    virtual int rx(uint16_t* data, int len)
    {
        HAL_StatusTypeDef okay;
        //okay = HAL_I2S_Receive_IT(& handle, data, len);
        okay = HAL_I2S_Receive(& handle, data, len, 100);
        return (okay == HAL_OK) ? len : -1;
    }
};

I2S * I2S::create(ID id)
{
    return new Arm_I2S(id);
}

}   //  namespace panglos

extern "C" void HAL_I2S_RxHalfCpltCallback(I2S_HandleTypeDef *hi2s)
{
    ASSERT(0);
}

extern "C" void HAL_I2S_RxCpltCallback(I2S_HandleTypeDef *hi2s)
{
    ASSERT(0);
}

//extern "C" void HAL_I2S_ErrorCallback(I2S_HandleTypeDef *hi2s)
//{
//    ASSERT(0);
//}

//  FIN
