
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
        Severity severity;

        static struct Logger **get_next(struct Logger *item);
        static int match_out(struct Logger *logger, void *out);
    };

    Severity severity;
    List<struct Logger*> loggers;
    Mutex *mutex;
    struct Logger *irq_logger;

public:
    Logging(Severity s);
    ~Logging();

    Severity set_severity(Severity s);
    void add(Out *out, Severity s);
    void add_irq(Out *out, Severity s);
    bool remove(Out *out);

    void log(Severity s, const char *fmt, va_list ap);
    int count();
};

extern Logging *logger;

}   //  namespace panglos

#endif  //  __PANGLOS_LOGGER__

//  FIN
