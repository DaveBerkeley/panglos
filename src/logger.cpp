
#include "panglos/debug.h"

#include "panglos/arch.h"
#include "panglos/io.h"

#include "panglos/logger.h"

    /*
     *
     */

namespace panglos {

Logging *logger = 0;

    /*
     *  Logger List<> primitives
     */

struct Logging::Logger **Logging::Logger::get_next(struct Logging::Logger *item)
{
    return & item->next;
}

int Logging::Logger::match_out(struct Logging::Logger *logger, void *arg)
{
    ASSERT(arg);
    Out *out = (Out*) arg;
    return logger->out == out;
}

    /*
     *
     */

Logging::Logging(Severity s)
:   severity(s),
    loggers(Logger::get_next),
    mutex(0),
    irq_logger(0)
{
    mutex = Mutex::create();
}

Logging::~Logging()
{
    while (loggers.head)
    {
        struct Logger *logger = loggers.pop(0);
        if (!logger)
        {
            break;
        }
        delete logger;
    }
    delete irq_logger;

    delete mutex;
}

    /*
     *
     */

void Logging::add_irq(Out *out, Severity s)
{
    ASSERT(!irq_logger); // only one allowed
    struct Logger *logger = new struct Logger;
    logger->next = 0;
    logger->out = out;
    logger->severity = s;

    irq_logger = logger;
}

void Logging::add(Out *out, Severity s)
{
    struct Logger *logger = new struct Logger;
    logger->next = 0;
    logger->out = out;
    logger->severity = s;

    loggers.push(logger, mutex);
}

bool Logging::remove(Out *out)
{
    Lock lock(mutex);

    struct Logger *logger = loggers.find(Logger::match_out, out, 0);

    if (!logger)
    {
        return false;
    }

    loggers.remove(logger, 0);
    delete logger;
    return true;
}

    /*
     *
     */

void Logging::log(Severity s, const char *fmt, va_list ap)
{
    if (s > severity)
    {
        return;
    }

    if (arch_in_irq())
    {
        if (irq_logger && (s <= irq_logger->severity))
        {
            FmtOut formatter(irq_logger->out, 0);
            formatter.printf(fmt, ap);
        }
        return;
    }

    Lock lock(mutex);

    for (struct Logger *logger = loggers.head; logger; logger = logger->next)
    {
        if (s > logger->severity)
        {
            continue;
        }

        FmtOut formatter(logger->out, 0);
        formatter.printf(fmt, ap);
    }
}

    /*
     *
     */

int Logging::count()
{
    return loggers.size(mutex);
}

    /*
     *
     */

void Logging::printf(Logging *logging, Severity s, const char *fmt, ...)
{
    ASSERT(logging);
    va_list ap;
    va_start(ap, fmt);
    logging->log(s, fmt, ap);
    va_end(ap);
}

}   //  namespace panglos

//  FIN
