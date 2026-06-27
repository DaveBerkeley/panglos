
static inline void yield_from_isr()
{
#if defined(ESP32)
    portYIELD_FROM_ISR();
#elif defined(STM32)
    portYIELD_FROM_ISR(0);
#else
#error "Different FreeRTOS Port needed"
#endif
}


