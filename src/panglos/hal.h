
    /*
     *  Provides target specific functions
     */

#if defined(ESP32)

#include "panglos/esp32/hal.h"

#elif defined(STM32)

#include "panglos/stm32/hal.h"

#else

#error "no platform defined"

#endif

//  FIN
