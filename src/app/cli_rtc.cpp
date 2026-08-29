
#if defined(PO_RTC)

#include "cli/src/cli.h"

#include "panglos/debug.h"
#include "panglos/drivers/rtc.h"
#include "panglos/object.h"

namespace panglos {

static RTC* (*lookup_fn)(const char *name, void *arg) = 0;
static void *lookup_fn_arg = 0;

static RTC *get_rtc(CLI *cli, const char *name)
{
    ASSERT(lookup_fn);
    RTC *rtc = lookup_fn(name, lookup_fn_arg);
    if (!rtc)
    {
        cli_print(cli, "RTC '%s' not found%s", name, cli->eol);
    }
    return rtc;
}

    /*
     *
     */

static void cmd_rtc_get(CLI *cli, CliCommand *)
{
    RTC *rtc = get_rtc(cli, "rtc");
    ASSERT(rtc);

    RTC::DateTime dt;

    if (!rtc->get(& dt))
    {
        cli_print(cli, "Error reading RTC%s", cli->eol);
        return;
    }

    cli_print(cli, "%04d-%02d-%02d %02d:%02d:%02d%s", dt.yy, dt.mm, dt.dd, dt.h, dt.m, dt.s, cli->eol);
}

static void cmd_rtc_set(CLI *cli, CliCommand *)
{
    RTC *rtc = get_rtc(cli, "rtc");
    ASSERT(rtc);

    int args[6];

    for (int i = 0; i < 6; i++)
    {
        const char *s = cli_get_arg(cli, i);
        if (!s)
        {
            cli_print(cli, "error reading arg=%d%s", i, cli->eol);
            return;
        }
        if (!cli_parse_int(s, & args[i], 0))
        {
            cli_print(cli, "error parsing arg=%d%s", i, cli->eol);
            return;
        }
    }

    RTC::DateTime dt = {
        .yy  = (uint16_t) args[0],
        .mm = (uint8_t) args[1],
        .dd   = (uint8_t) args[2],
        .h  = (uint8_t) args[3],
        .m   = (uint8_t) args[4],
        .s   = (uint8_t) args[5],
    };

    if (!rtc->validate(dt))
    {
        cli_print(cli, "Invalid date-time%s", cli->eol);
        return;
    }

    //cli_print(cli, "%04d-%02d-%02d %02d:%02d:%02d%s", dt.year, dt.month, dt.day, dt.hour, dt.min, dt.sec, cli->eol);
    if (!rtc->set(& dt))
    {
        cli_print(cli, "Error writing RTC%s", cli->eol);
        return;
    }
}

static CliCommand rtc_set  = { "set", cmd_rtc_set, .help="set y m d h m s", };
static CliCommand rtc_get  = { "get", cmd_rtc_get, .next=& rtc_set, };
static CliCommand rtc_cmd  = { "rtc", .subcommand=& rtc_get };

void add_cli_rtc_cmd(CLI *cli, RTC* (*fn)(const char *name, void *), void *arg)
{
    lookup_fn = fn;
    lookup_fn_arg = arg;
    cli_insert(cli, & cli->head, & rtc_cmd);
}

}   //  namespace panglos

#endif // (PO_RTC)

//  FIN
