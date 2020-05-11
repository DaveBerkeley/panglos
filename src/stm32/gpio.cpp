
#if defined(STM32F1xx)
#include "stm32f1xx_hal.h"
#else
#include "stm32f4xx_hal.h"
#endif

#include "../debug.h"
#include "../gpio.h"
#include "gpio_arm.h"

    /*
     *  GPIO
     */

static inline int pin_to_idx(uint16_t pin)
{
    switch (pin)
    {
        case GPIO_PIN_0  :  return 0;
        case GPIO_PIN_1  :  return 1;
        case GPIO_PIN_2  :  return 2;
        case GPIO_PIN_3  :  return 3;
        case GPIO_PIN_4  :  return 4;
        case GPIO_PIN_5  :  return 5;
        case GPIO_PIN_6  :  return 6;
        case GPIO_PIN_7  :  return 7;
        case GPIO_PIN_8  :  return 8;
        case GPIO_PIN_9  :  return 9;
        case GPIO_PIN_10 :  return 10;
        case GPIO_PIN_11 :  return 11;
        case GPIO_PIN_12 :  return 12;
        case GPIO_PIN_13 :  return 13;
        case GPIO_PIN_14 :  return 14;
        case GPIO_PIN_15 :  return 15;
    }
    ASSERT(0);
    return 0;
}

namespace panglos {

    /*
     *
     */

class ARM_GPIO;

static ARM_GPIO *interrupt_table[16];

    /*
     *
     */

class ARM_GPIO : public panglos::GPIO
{
    GPIO_TypeDef *port;
    uint16_t pin;
    void (*irq_handler)(void *arg);
    void *irq_arg;
    bool irq_enabled;
public:

    ARM_GPIO(GPIO_TypeDef *_port, uint16_t _pin, uint32_t mode)
    :   port(_port), pin(_pin), irq_handler(0), irq_arg(0), irq_enabled(false)
    {
        GPIO_InitTypeDef gpio_init = {0};

        set(false);

        gpio_init.Pin = pin;
        gpio_init.Speed = GPIO_SPEED_FREQ_LOW;
        gpio_init.Mode = mode;
        gpio_init.Pull = GPIO_NOPULL;

        HAL_GPIO_Init(port, & gpio_init);

        // Tutorial on GPIO interrupts :
        // https://stm32f4-discovery.net/2014/08/stm32f4-external-interrupts-tutorial/

        switch (mode)
        {
            case GPIO_MODE_IT_RISING :
            case GPIO_MODE_IT_FALLING :
            case GPIO_MODE_IT_RISING_FALLING :
                // proceed to initialise the interrupt
                break;
            default:
                // done, no further initialisation
                return;
        }

        IRQn_Type irq_type = EXTI0_IRQn;

        // For the stm32f405xx we have EXTI0_IRQn .. EXTI4_IRQn, EXTI9_5_IRQn, EXTI15_10_IRQn
        // TODO : does this depend on the processor version?
        switch (pin)
        {
            case GPIO_PIN_0  :  irq_type = EXTI0_IRQn; break;
            case GPIO_PIN_1  :  irq_type = EXTI1_IRQn; break;
            case GPIO_PIN_2  :  irq_type = EXTI2_IRQn; break;
            case GPIO_PIN_3  :  irq_type = EXTI3_IRQn; break;
            case GPIO_PIN_4  :  irq_type = EXTI4_IRQn; break;
            case GPIO_PIN_5  :  // fall through
            case GPIO_PIN_6  :  // fall through
            case GPIO_PIN_7  :  // fall through
            case GPIO_PIN_8  :  // fall through
            case GPIO_PIN_9  :  irq_type = EXTI9_5_IRQn; break;
            case GPIO_PIN_10 :  // fall through
            case GPIO_PIN_11 :  // fall through
            case GPIO_PIN_12 :  // fall through
            case GPIO_PIN_13 :  // fall through
            case GPIO_PIN_14 :  // fall through
            case GPIO_PIN_15 :  irq_type = EXTI15_10_IRQn; break;
            default : ASSERT(0);
        }

        // Note :- priority should be >= configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY
        HAL_NVIC_SetPriority(irq_type, 6, 0);
        HAL_NVIC_EnableIRQ(irq_type);

        const int idx = pin_to_idx(pin);
        // don't currently support more than one interrupt on the same pin number
        ASSERT(!interrupt_table[idx]);
        interrupt_table[idx] = this;
        irq_enabled = true;
    }

    virtual void set(bool state)
    {
        HAL_GPIO_WritePin(port, pin, state ? GPIO_PIN_SET : GPIO_PIN_RESET);
    }

    virtual bool get()
    {
        return HAL_GPIO_ReadPin(port, pin);
    }

    virtual void toggle()
    {
        HAL_GPIO_TogglePin(port, pin);
    }

    virtual void set_interrupt_handler(void (*fn)(void *arg), void *arg)
    {
        ASSERT(irq_enabled); // mode must be GPIO_MODE_IT_*
        irq_handler = fn;
        irq_arg = arg;
    }

    virtual void on_interrupt()
    {
        if (irq_handler)
        {
            irq_handler(irq_arg);
        }
    }
};

GPIO * gpio_create(GPIO_TypeDef *port, uint16_t pin, uint32_t mode)
{
    return new ARM_GPIO(port, pin, mode);
}

}   //  namespace panglos

    /*
     *
     */

extern "C" void HAL_GPIO_EXTI_Callback(uint16_t pin)
{
    // All GPIO interrupts cause this callback
    panglos::ARM_GPIO *gpio = panglos::interrupt_table[pin_to_idx(pin)];
    ASSERT(gpio);
    gpio->on_interrupt();
}

    /*
     *  Interrupt handlers for all pins
     */

extern "C" void EXTI0_IRQHandler(void)
{
    if (__HAL_GPIO_EXTI_GET_FLAG(GPIO_PIN_0))
    {
        HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_0);
    }
}

extern "C" void EXTI1_IRQHandler(void)
{
    if (__HAL_GPIO_EXTI_GET_FLAG(GPIO_PIN_1))
    {
        HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_1);
    }
}

extern "C" void EXTI2_IRQHandler(void)
{
    if (__HAL_GPIO_EXTI_GET_FLAG(GPIO_PIN_2))
    {
        HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_2);
    }
}

extern "C" void EXTI3_IRQHandler(void)
{
    if (__HAL_GPIO_EXTI_GET_FLAG(GPIO_PIN_3))
    {
        HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_3);
    }
}

extern "C" void EXTI4_IRQHandler(void)
{
    if (__HAL_GPIO_EXTI_GET_FLAG(GPIO_PIN_4))
    {
        HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_4);
    }
}

extern "C" void EXTI15_10_IRQHandler(void)
{
    const uint16_t pins[] = {
        GPIO_PIN_10,
        GPIO_PIN_11,
        GPIO_PIN_12,
        GPIO_PIN_13,
        GPIO_PIN_14,
        GPIO_PIN_15,
        0
    };

    for (int i = 0; pins[i]; i++)
    {
        const uint16_t pin = pins[i];
        if (__HAL_GPIO_EXTI_GET_FLAG(pin))
        {
            HAL_GPIO_EXTI_IRQHandler(pin);
        }
    }
}

extern "C" void EXTI9_5_IRQHandler(void)
{
    const uint16_t pins[] = {
        GPIO_PIN_5,
        GPIO_PIN_6,
        GPIO_PIN_7,
        GPIO_PIN_8,
        GPIO_PIN_9,
        0
    };

    for (int i = 0; pins[i]; i++)
    {
        const uint16_t pin = pins[i];
        if (__HAL_GPIO_EXTI_GET_FLAG(pin))
        {
            HAL_GPIO_EXTI_IRQHandler(pin);
        }
    }
}

// FIN
