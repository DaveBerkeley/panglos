
#include "panglos/debug.h"

#include "panglos/stm32/gpio_f1.h"

#if defined(STM32F1)

class _GPIO : public STM32F1_GPIO
{
private:
    static uint32_t ck_enabled;

    GPIO_TypeDef *base;
    uint32_t mask;

    static void enable_port_power(GPIO_TypeDef *base)
    {
        uint32_t pwr_mask = 0;

        switch ((uint32_t) base)
        {
            case GPIOA_BASE : pwr_mask = RCC_APB2ENR_IOPAEN; break;
            case GPIOB_BASE : pwr_mask = RCC_APB2ENR_IOPBEN; break;
            case GPIOC_BASE : pwr_mask = RCC_APB2ENR_IOPCEN; break;
            case GPIOD_BASE : pwr_mask = RCC_APB2ENR_IOPDEN; break;
            default : ASSERT(0);
        }
        if (!(ck_enabled & pwr_mask))
        {
            RCC->APB2ENR |= pwr_mask;
            ck_enabled |= pwr_mask;
        }
    }

public:
    _GPIO(GPIO_TypeDef *_base, uint32_t pin, IO io)
    :   base(_base),
        mask(1 << pin)
    {
        init(_base, pin, io);
    }

    static void init(GPIO_TypeDef *base, uint32_t pin, IO io)
    {
        enable_port_power(base);

        // TODO : give access to Speed setting? eg Max speed 10/2/50 MHz
        const uint8_t mode = (io & INPUT) ? 0 : 0x3; // 0x3 = 50MHz

        uint8_t cfg = 0;

        if (io & INPUT)
        {
            cfg |= 0x01; // floating input
            if (io & (PULL_UP | PULL_DOWN))
            {
                cfg |= 0x02;
            }
        }

        if (io & OUTPUT)
        {
            if (io & OPEN_DRAIN)
            {
                cfg |= 0x01;
            }
            if (io & ALT)
            {
                cfg |= 0x02;
            }
        }

        // 4 bits in control reg for cfg[1:0] and mode[1:0]
        const uint8_t d = (cfg << 2) | mode;
        const uint8_t cfg_mask = 0xf;
        const uint8_t shift = (pin % 8) * 4;
        // CRL for pins 0..7, CRH for 8..15
        volatile uint32_t *reg = (pin < 8) ? & base->CRL : & base->CRH;
        *reg &= ~(cfg_mask << shift);
        *reg |= d << shift;
    }

    virtual void set(bool state) override
    {
        volatile uint32_t *reg = state ? & base->BSRR : & base->BRR;
        *reg = mask;
    }
    virtual bool get() override
    {
        return base->IDR & mask;
    }
    virtual void toggle() override
    {
        set(!get());
    }
};

uint32_t _GPIO::ck_enabled = false;

panglos::GPIO *STM32F1_GPIO::create(GPIO_TypeDef *_base, uint32_t _pin, IO io)
{
    if (io & INIT_ONLY)
    {
        // don't need to allocate any memory, just run the init code
        _GPIO::init(_base, _pin, io);
        return 0;
    }

    return new _GPIO(_base, _pin, io);
}

#endif  //  STM32F1

//  FIN
