
#if defined(PO_SPI)

#include <string.h>

#include "cli/src/cli.h"

#include "panglos/debug.h"

#include "panglos/io.h"
#include "panglos/object.h"
#include "panglos/drivers/spi.h"
#include "panglos/drivers/gpio.h"
#include "panglos/app/cli_spi.h"

namespace panglos {

static GPIO* (*lookup_fn)(const char *name, void *arg) = 0;
static void *lookup_fn_arg = 0;

    /*
     *
     */

static void cmd_spi(CLI *cli, CliCommand *cmd)
{
    SPI *spi = (SPI*) Objects::objects->get("spi");
    if (!spi)
    {
        cli_print(cli, "unable to find spi device%s", cli->eol);
        return;
    }

    //  "spi <cs> <data> [<data>]*"
    int idx = 0;
    const char *s = cli_get_arg(cli, idx++);

    GPIO *cs = (GPIO*) Objects::objects->get(s);
    if (!cs)
    {
        cli_print(cli, "unable to find cs '%s'%s", s, cli->eol);
        return;
    }

    uint8_t wr[16];
    uint8_t rd[16];
    int n = 0;

    // Read N args into wr[]
    for (size_t i = 0; i < sizeof(wr); i++)
    {
        s = cli_get_arg(cli, idx);
        if (!s) break;
        int value;
        if (!cli_parse_int(s, & value, 0))
        {
            cli_print(cli, "bad value '%s' at idx=%d%s", s, idx, cli->eol);
            return;
        }
        wr[i] = uint8_t(value);
        n += 1;
        idx += 1;
    }

    enum State { SPI_WR, SPI_IO, SPI_NONE };
    enum State state = SPI_NONE;
    if (!strcmp("wr", cmd->cmd)) state = SPI_WR;
    if (!strcmp("io", cmd->cmd)) state = SPI_IO;

    const char *error = 0;

    {
        SPI::CS_Lock lock(spi, cs);
        switch (state)
        {
            case SPI_WR :
            {
                if (!spi->write(wr, n))
                    error = "Error writing to spi";
                break;
            }
            case SPI_IO :
            {
                if (!spi->io(wr, rd, n))
                    error = "Error reading/writing to spi";
                break;
            }
            case SPI_NONE :
            default : ASSERT(0);
        }
    }

    if (error)
    {
        cli_print(cli, "%s%s", error, cli->eol);
        return;
    }

    cli_print(cli, "wr: ");
    for (int i = 0; i < n; i++)
    {
        cli_print(cli, "%02x ", wr[i]);
    }
    cli_print(cli, "%s", cli->eol);

    if (state == SPI_IO)
    {
        cli_print(cli, "rd: ");
        for (int i = 0; i < n; i++)
        {
            cli_print(cli, "%02x ", rd[i]);
        }
        cli_print(cli, "%s", cli->eol);
    }

    cli_print(cli, "%s", cli->eol);
}

static void cmd_spi_bad(CLI *cli, CliCommand *)
{
    cli_print(cli, "expected wr|io <cs> ...%s", cli->eol);
}

static CliCommand spi_wr_cmd = { .cmd="wr", .handler=cmd_spi, .help="spi wr <cs> <data> [<data>]*", };
static CliCommand spi_io_cmd = { .cmd="io", .handler=cmd_spi, .help="spi io <cs> <data> [<data>]*", .next = & spi_wr_cmd };
static CliCommand spi_cmd = { .cmd="spi", .handler=cmd_spi_bad, .subcommand = & spi_io_cmd };

    /*
     *
     */

void add_cli_spi_cmd(CLI *cli, GPIO* (*fn)(const char *name, void *), void *arg)
{
    lookup_fn = fn;
    lookup_fn_arg = arg;
    cli_insert(cli, & cli->head, & spi_cmd);
}

}   //  namespace panglos

#endif // defined(PO_SPI)

//  FIN
