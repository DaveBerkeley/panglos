
#include "panglos/drivers/gpio.h"

#if defined(STM32F1)

#include "stm32f1xx.h"

class STM32F1_GPIO : public panglos::GPIO
{
public:
    typedef enum {
        INPUT = 0x01, 
        OUTPUT = 0x02,
        PULL_UP = 0x04,
        PULL_DOWN = 0x08,
        OPEN_DRAIN = 0x10,
        ALT = 0x20,
        INIT_ONLY = 0x40,
    } IO;

    static panglos::GPIO* create(GPIO_TypeDef *_base, uint32_t _pin, IO io);
};

#endif  //  STM32F1

//  FIN
