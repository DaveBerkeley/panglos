
#if defined(USE_FREE_RTOS)

#include <string.h>

#include <FreeRTOS.h>

extern void Error_Handler(void);

    /*
     *  Implement malloc/free using FreeRTOS thread safe calls.
     *
     *  The C++ new operator calls malloc().
     *
     *  To adjust the available heap size, edit configTOTAL_HEAP_SIZE
     *  in Core/Inc/FreeRTOSConfig.h
     */

void* malloc(size_t size)
{
    return pvPortMalloc(size);
}

void* calloc(size_t nmemb, size_t size)
{
    const size_t s = size * nmemb;
    void *m = malloc(s);
    memset(m, 0, s);
    return m;
}

void free(void *s)
{
    vPortFree(s);
}

void vApplicationMallocFailedHook()
{
    Error_Handler();
}

#endif  //  USE_FREE_RTOS

//  FIN
