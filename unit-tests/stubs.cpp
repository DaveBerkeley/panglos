
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include "panglos/debug.h"
#include "panglos/thread.h"

using namespace panglos;

extern "C" {

const LUT Severity_lut[] = {
    { "NONE", S_NONE, },
    { "CRITICAL", S_CRITICAL, },
    { "ERROR", S_ERROR, },
    { "WARNING", S_WARNING, },
    { "NOTICE", S_NOTICE, },
    { "INFO", S_INFO, },
    { "DEBUG", S_DEBUG, },
    { 0, S_NONE},
};

void Error_Handler(void)
{
    printf("ERROR\n");
    exit(0);
}

uint32_t get_time(void)
{
    // TODO
    return 0x1234;
}

const char *get_task(void)
{
    Thread *thread = Thread::get_current();
    return thread ? thread->get_name() : "unknown";
}

void po_log(Severity s, const char *fmt, ...)
{
    IGNORE(s);
    va_list ap;
    va_start(ap, fmt);
    vprintf(fmt, ap);
    va_end(ap);
}

}   //  extern "C"

//  FIN
