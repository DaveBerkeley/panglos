
#if !defined(__PANGLOS_LOGGER__)
#define __PANGLOS_LOGGER__

#include <stdarg.h>

#include "panglos/list.h"

    /*
     *
     */

namespace panglos {

    /*
     *
     */

class Out;
class Mutex;

class Logging
{
    struct Logger {
        struct Logger *next;
        Out *out;
        Mutex *mutex;
        Severity severity;

        static struct Logger **get_next(struct Logger *item);
        static int match_out(struct Logger *logger, void *out);
    };

    Severity severity;
    List<struct Logger*> loggers;
    struct Logger *irq_logger;
    Mutex *mutex;

public:
    Logging(Severity s, Mutex *mutex);
    ~Logging();

    Severity set_severity(Severity s);
    void add(Out *out, Severity s, Mutex *mutex);
    void add_irq(Out *out, Severity s);
    bool remove(Out *out);

    void log(Severity s, const char *fmt, va_list ap);
    int count();

    static void printf(Logging *logging, Severity s, const char *fmt, ...)
                __attribute__((format(printf, 3, 4)));
};

extern Logging *logger;

}   //  namespace panglos

#endif  //  __PANGLOS_LOGGER__

//  FIN
