
    /*
     *  Provides target specific functions
     */

#if defined(ESP32)
#include "panglos/esp32/hal.h"
#elif defined(STM32)
// TODO
#else
#error "no platform defined"
#endif

//  FIN
