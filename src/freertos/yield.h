
static inline void yield_from_isr()
{
#if defined(ESP32)
    portYIELD_FROM_ISR();
#else
#error "Different FreeRTOS Port needed"
#endif
}


