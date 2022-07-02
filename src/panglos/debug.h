
#if !defined(__DEBUG_H__)
#define __DEBUG_H__

#include <stdint.h>

#define IGNORE(x) ((void) (x))

typedef enum {
    S_NONE = 0,
    /// Critical: critical conditions
    S_CRITICAL = 2,
    /// Error: error conditions
    S_ERROR = 3,
    /// Warning: warning conditions
    S_WARNING = 4,
    /// Notice: normal but significant condition
    S_NOTICE = 5,
    /// Informational: informational messages
    S_INFO = 6,
    /// Debug: debug-level messages
    S_DEBUG = 7,
}   Severity;

    /*
     *
     */

typedef struct {
    const char *text;
    int code;
}   LUT;

inline const char *lut(const LUT *codes, int err)
{
    for (const LUT *code = codes; code->text; code++)
    {
        if (code->code == err)
        {
            return code->text;
        }
    }

    return "unknown";
}

#if defined(__cplusplus)
extern "C" {
#endif

void Error_Handler(void);
uint32_t get_time(void);
const char *get_task(void);
void po_log(Severity s, const char *fmt, ...) __attribute__((format(printf,2,3)));

extern const LUT Severity_lut[];

#if defined(__cplusplus)
}
#endif

    /*
     *
     */

#define _PO_PRINT(level, fmt, ...) \
        po_log(level, "%d %s %s %s +%d %s() : " fmt "\r\n", \
                get_time(), get_task(), \
                lut(Severity_lut, level), \
                __FILE__, __LINE__, __FUNCTION__, \
                ## __VA_ARGS__ );

#define PO_DEBUG(fmt, ...)      _PO_PRINT(S_DEBUG, fmt, ## __VA_ARGS__ )
#define PO_ERROR(fmt, ...)      _PO_PRINT(S_ERROR, fmt, ## __VA_ARGS__ )
#define PO_INFO(fmt, ...)       _PO_PRINT(S_INFO, fmt, ## __VA_ARGS__ )
#define PO_WARNING(fmt, ...)   _PO_PRINT(S_WARNING, fmt, ## __VA_ARGS__ )

#define ASSERT(x)           if (!(x)) { PO_ERROR("assert failed"); Error_Handler(); }
#define ASSERT_ERROR(x,fmt,...) \
                            if (!(x)) {  _PO_PRINT(S_ERROR, fmt, ## __VA_ARGS__ ); Error_Handler(); }

#endif // __DEBUG_H__

//  FIN
