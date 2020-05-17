
#if !defined(__DEBUG_H__)
#define __DEBUG_H__

#define IGNORE(x) ((x) = (x))

    /*
     *
     */

namespace panglos {
    void * get_task_id();

    /*
     *
     */

typedef struct {
    int code;
    const char *text;
}   Code;

inline const char *err_lookup(const Code *codes, int err, const char *def="unknown")
{
    for (const Code *code = codes; code->text; code++)
    {
        if (code->code == err)
        {
            return code->text;
        }
    }

    return def;
}

}


#if defined(GTEST)

#include <assert.h>
#include <stdio.h>

#define ASSERT(x) assert(x)

#define PO_DEBUG(x, ...)    fprintf(stderr, "DEBUG %p %s %d %s " x "\r\n", panglos::get_task_id(), __FILE__, __LINE__, __PRETTY_FUNCTION__,  ## __VA_ARGS__ )
#define PO_ERROR(x, ...)    fprintf(stderr, "ERROR %p %s %d %s " x "\r\n", panglos::get_task_id(), __FILE__, __LINE__, __PRETTY_FUNCTION__,  ## __VA_ARGS__ )
#define PO_REPORT(x, ...)   fprintf(stderr, "REPORT %p %s %d %s " x "\r\n", panglos::get_task_id(), __FILE__, __LINE__, __PRETTY_FUNCTION__,  ## __VA_ARGS__ )

#else // GTEST

    /*
     *
     */

#include "uart.h"
#include "timer.h"

extern "C" void Error_Handler(void);

extern panglos::Output *err_uart;

#define _PO_PRINT(uart, level, fmt, ...) \
    if (uart) { \
        uart->printf("0x%08x %#p " level " %s +%d %s() : " fmt "\r\n", \
                panglos::timer_now(), panglos::get_task_id(), __FILE__, __LINE__, __FUNCTION__, \
                ## __VA_ARGS__ ); \
    }

#define PO_DEBUG(fmt, ...) _PO_PRINT(err_uart, "DEBUG", fmt, ## __VA_ARGS__ )
#define PO_ERROR(fmt, ...) _PO_PRINT(err_uart, "ERROR", fmt, ## __VA_ARGS__ )

#define PO_REPORT(fmt, ...) if (err_uart) { err_uart->printf(fmt "\r\n", ## __VA_ARGS__ ); }

#define ASSERT(x) if (!(x)) { PO_ERROR("assert failed"); Error_Handler(); }

#endif // GTEST

#endif // __DEBUG_H__

//  FIN
