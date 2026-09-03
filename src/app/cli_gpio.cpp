
#include <string.h>

#include <atomic>

#include "cli/src/cli.h"

#include "panglos/debug.h"
#include "panglos/object.h"
#include "panglos/time.h"

#include "panglos/drivers/gpio.h"

namespace panglos {

#if defined(GTEST)
// need this to disable the clang sanitiser warnings in CliCmd tests
std::atomic<bool> gpio_end;
#else
static bool gpio_end;
#endif

void gpio_terminate_loop()
{
    gpio_end = true;
}

static void gpio_flash(CLI *cli, int idx)
{
    int delay = 500; //ms
    const char *s = cli_get_arg(cli, idx);
    if (cli_parse_int(s, & delay, 0))
    {
        cli_print(cli, "flash rate=%d ms%s", delay, cli->eol);
        idx += 1;
    }
    while (!gpio_end)
    {
        for (int i = idx; ; i++)
        {
            s = cli_get_arg(cli, i);
            if (!s)
                break;
            GPIO *gpio = (GPIO*) Objects::objects->get(s);
            if (!gpio)
                break;
            gpio->toggle();
        }
        Time::msleep(delay);
    }
}

static void cmd_gpio(CLI *cli, CliCommand *)
{
    bool toggle = false;
    bool show = false;
    int idx = 0;
    int delay = 500; //ms

    gpio_end = false;
    const char *s = cli_get_arg(cli, idx++);

    if (s && !strcmp(s, "toggle"))
    {
        toggle = true;
        s = cli_get_arg(cli, idx++);
    }

    if (s && !strcmp(s, "show"))
    {
        show = true;
        s = cli_get_arg(cli, idx++);
        if (cli_parse_int(s, & delay, 0))
        {
            s = cli_get_arg(cli, idx++);
        }
    }

    if (s && !strcmp(s, "flash"))
    {
        gpio_flash(cli, idx);
        return;
    }

    if (!s)
    {
        cli_print(cli, "expects name of gpio%s", cli->eol);
        return;
    }

    const char *name = s;
    GPIO *gpio = (GPIO*) Objects::objects->get(s);
    if (!gpio)
    {
        cli_print(cli, "can't find device %s'%s", s, cli->eol);
        return;
    }

    if (toggle)
    {
        gpio->toggle();
        return;
    }

    // if "gpio show [delay_ms] <name>" loop forever printing value
    if (show)
    {

        while (!gpio_end)
        {
            cli_print(cli, "%s %d%s", name, gpio->get(), cli->eol);
            Time::msleep(delay);
        }
        return;
    }

    s = cli_get_arg(cli, idx++);
    if (!s)
    {
        cli_print(cli, "expects state 0|1|?%s", cli->eol);
        return;
    }

    if (!strcmp(s, "?"))
    {
        cli_print(cli, "%d%s", gpio->get(), cli->eol);
        return;
    }

    int value;
    if (!cli_parse_int(s, & value, 0))
    {
        cli_print(cli, "expects value [0,1]%s", cli->eol);
        return;
    }

    gpio->set(value);
}

    /*
     *
     */

static CliCommand gpio = { "gpio",   cmd_gpio,   "gpio [show|toggle|flash] <gpio> [0|1|?]", 0, 0, 0 };

void add_cli_gpio_cmd(CLI *cli, GPIO* (*lookup_fn)(const char *name, void *), void *arg)
{
    // TODO
    IGNORE(lookup_fn);
    IGNORE(arg);
    cli_insert(cli, & cli->head, & gpio);
}

}   //  namespace panglos

//  FIN
