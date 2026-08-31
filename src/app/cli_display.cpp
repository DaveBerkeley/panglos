
#include <string.h>

#include "cli/src/cli.h"

#include "panglos/debug.h"

#include "panglos/io.h"
#include "panglos/drivers/7-segment.h"
#include "panglos/app/cli_display.h"


namespace panglos {

static Display* (*lookup_fn)(const char *name, void *arg) = 0;
static void *lookup_fn_arg = 0;

static void cmd_display(CLI *cli, CliCommand *)
{
    Display *display = lookup_fn(0, lookup_fn_arg);
    if (!display)
    {
        cli_print(cli, "no display found%s", cli->eol);
        return;
    }

    char buff[16] = { 0 };

    int idx = 0;
    while (true)
    {
        const char *s = cli_get_arg(cli, idx++);
        if (!s) break;
        strcat(buff, s);
        strcat(buff, " ");
    }
    display->write(buff);
}

static CliCommand _display = { .cmd="display", .handler=cmd_display };

void add_cli_display_cmd(CLI *cli, Display* (*fn)(const char *name, void *), void *arg)
{
    lookup_fn = fn;
    lookup_fn_arg = arg;
    cli_insert(cli, & cli->head, & _display);
}

}   //  namespace panglos

//  FIN
