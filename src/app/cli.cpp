
#include "cli/src/cli.h"

#include "panglos/debug.h"
#include "panglos/time.h"
#include "panglos/object.h"
#include "panglos/queue.h"
#include "panglos/logger.h"
#include "panglos/mutex.h"
#include "panglos/event_queue.h"
#include "panglos/watchdog.h"
#include "panglos/device.h"
#include "panglos/drivers/gpio.h"
#include "panglos/drivers/uart.h"

#include "panglos/app/cli.h"
#include "panglos/app/event.h"
#include "panglos/app/cli_cmd.h"

namespace panglos {

    /*
     *
     */

static CliCommand *cmds = 0;

void add_cli_command(CliCommand *cmd)
{
    PO_DEBUG("%p %s", cmd, cmd->cmd);
    cmd->next = cmds;
    cmds = cmd;
}

    /*
     *
     */

class CliOut : public panglos::Out
{
    virtual int tx(const char* data, int n) override
    {
        for (int i = 0; i < n; i++)
        {
            putchar(*data++);
        }
        return n;
    }
};

    /*
     *
     */

void run_cli()
{
#if defined(FREERTOS)
    Mutex *mutex = 0;
#else
    Mutex *mutex = Mutex::create();
#endif
    Event::Queue *queue = Event::create_queue(10, mutex);
    ASSERT(queue);

    Objects::objects->add("cli_queue", queue);

    // If the CLI is on a separate UART, not stdio
    UART *uart = (UART*) Objects::objects->get("cli_uart");
    PO_DEBUG("cli_uart=%p", uart);

    Out *out = uart;
    if (!out)
    {
        static CliOut cli_out;
        out = & cli_out;
    }

    FmtOut fmt_out(out, 0);

    static CLI cli { 0 };

    CliOutput output = {
        .fprintf = FmtOut::xprintf,
        .ctx = & fmt_out,
    };

    cli.output = & output;
    cli.eol = "\r\n";
    cli.prompt = "> ";
    cli_init(& cli, 96, 0);

    const char *banner = (const char *) Objects::objects->get("banner");
    if (banner)
    {
        cli_print(& cli, "\r\n%s", banner);
    }
 
#if defined(ARCH_RISCV32)
    cli_print(& cli, "ARCH_RISCV32 %s", cli.eol);
#endif
#if defined(ARCH_XTENSA)
    cli_print(& cli, "ARCH_XTENSA %s", cli.eol);
#endif

    while (cmds)
    {
        CliCommand *c = cmds;
        cmds = c->next;
        c->next = 0;
        cli_register(& cli, c);
    }

    add_cli_commands(& cli);

    Objects::objects->add("cli", & cli);
    Objects::objects->add("cli_commands", cli.head);
 
    // Send an INIT event to all registered handlers
    {
        Event event;
        event.type = Event::INIT;
        EventHandler::handle_event(queue, & event);
        EventHandler::unlink_handlers(Event::INIT);
    }

    EvQueue *eq = (EvQueue*) Objects::objects->get("event_queue");

    GPIO *led = (GPIO*) Objects::objects->get("led");
    Time::tick_t led_time = 0;

    Watchdog *watchdog = (Watchdog*) Objects::objects->get("watchdog");

    while (true)
    {
        if (watchdog)
        {
            watchdog->poll(500);
        }

        if (eq)
        {
            eq->run(Time::get());
        }

        if (led && Time::elapsed(led_time, 100)) 
        {
            led_time = Time::get();
            led->toggle();
        }

        Event event;
        if (!event.get(queue, 1))
        {
            // read from stdin
            while (true)
            {
                const int c = getchar();
                if (c == -1)
                {
                    break;
                }
                cli_process(& cli, (c == '\r') ? '\n' : char(c));
            }

            // call IDLE event handlers
            event.type = Event::IDLE;
            EventHandler *eh = EventHandler::handle_event(queue, & event);
            if (eh)
            {
                EventHandler::del_handler(eh);
            }

            continue;
        }

        if (EventHandler::handle_event(queue, & event))
        {
            //PO_DEBUG("code=%d %s", event.type, lut(Event::type_lut, event.type));
            continue;
        }

        PO_ERROR("unhandled event=%s", lut(Event::type_lut, event.type));
    }
}

}   //  namespace panglos

//  FIN
