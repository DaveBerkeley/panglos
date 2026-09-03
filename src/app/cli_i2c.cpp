    /*
     *
     */

#if defined(PO_I2C)

#include <string.h>

#include "cli/src/cli.h"

#include "panglos/debug.h"

#include "panglos/io.h"
#include "panglos/drivers/i2c.h"
#include "panglos/app/cli_i2c.h"

namespace panglos {

static I2C* (*lookup_fn)(const char *name, void *arg) = 0;
static void *lookup_fn_arg = 0;

    /*
     *
     */

struct CliArgs
{
    I2C *i2c;
    uint8_t addr;

    struct Arg
    {
        uint8_t value;
        bool valid;
    };
    struct Arg args[16];

    int idx;
};

static bool check_needs(CLI *cli, struct CliArgs *args, bool needs_addr)
{
    if (!args->i2c)
    {
        cli_print(cli, "no device/addr specified%s", cli->eol);
        return false;
    }

    if (!needs_addr) return true;

    if (!args->addr)
    {
        cli_print(cli, "no device address%s", cli->eol);
        return false; 
    }

    if (!args->args[0].valid)
    {
        cli_print(cli, "no reg address%s", cli->eol);
        return false; 
    }

    return true;
}

    /*
     *
     */

static bool cli_i2c_parse(CLI *cli, struct CliArgs *args, bool needs_addr)
{
    // [dev] [addr [reg [n_regs]]]
    const char *s = cli_get_arg(cli, args->idx++);

    if (!s)
    {
        // no args : must be a raw scan with default dev and no addr
        args->i2c = lookup_fn(0, lookup_fn_arg);
        return check_needs(cli, args, needs_addr);
    }

    int value;
    if (cli_parse_int(s, & value, 0))
    {
        // no dev specified, use default
        args->i2c = lookup_fn(0, lookup_fn_arg);
        if (!args->i2c)
        {
            cli_print(cli, "no default device%s", cli->eol);
            return false;
        }
    }
    else
    {
        // if the arg isn't a number, it must be a dev name
        args->i2c = lookup_fn(s, lookup_fn_arg);
        // device not found
        if (!args->i2c)
        {
            cli_print(cli, "device '%s' not found%s", s, cli->eol);
            return false;
        }

        s = cli_get_arg(cli, args->idx++);
        if (!s) return check_needs(cli, args, needs_addr);

        if (!cli_parse_int(s, & value, 0))
        {
            cli_print(cli, "bad device address '0x%02x'%s", value, cli->eol);
            return false;
        }
    }

    ASSERT(args->i2c);
    args->addr = uint8_t(value);

    if (needs_addr & !args->addr)
    {
        cli_print(cli, "no device address%s", cli->eol);
        return false; 
    }

    // get further numbers
    for (int i = 0; i < 16; i++)
    {
        s = cli_get_arg(cli, args->idx++);
        if (!s)
        {
            break;
        }

        if (!cli_parse_int(s, & value, 0))
        {
            cli_print(cli, "bad value '%s'%s", s, cli->eol);
            return false;
        }
        args->args[i].value = uint8_t(value);
        args->args[i].valid = true;
    }

    return check_needs(cli, args, needs_addr);
}

    /*
     *
     */

static void cmd_i2c_scan(CLI *cli, CliCommand *)
{
    struct CliArgs args = { 0 };

    if (!cli_i2c_parse(cli, & args, false))
    {
        return;
    }

    uint8_t start = 0;
    uint8_t end = 0x7f;

    if (args.addr != 0)
    {
        start = end = uint8_t(args.addr);
    }

    // Scan range of addresses
    bool found = false;

    for (uint8_t addr = start; addr <= end; addr++)
    {
        const bool ok = args.i2c->probe(addr, 2);
        if (ok)
        {
            cli_print(cli, "found %#x%s", addr, cli->eol);
            found = true;
        }
    }

    if (found) return;

    if (args.addr)
    {
        cli_print(cli, "no probe response at 0x%02x%s", args.addr, cli->eol);
    }
    else
    {
        cli_print(cli, "no probe response%s", cli->eol);
    }
}

    /*
     *
     */

static void cmd_i2c_rd(CLI *cli, CliCommand *)
{
    struct CliArgs args = { 0 };

    if (!cli_i2c_parse(cli, & args, true))
    {
        return;
    }

    const uint8_t reg = args.args[0].value;
    const int n_args = args.args[1].valid ? args.args[1].value : 1;

    uint8_t data[16];
    if (!args.i2c->write_read(args.addr, & reg, sizeof(reg), data, uint32_t(n_args)))
    {
        cli_print(cli, "error reading device 0x%02x at address 0x%02x%s", args.addr, reg, cli->eol);
        return;
    }

    cli_print(cli, "dev=0x%02x 0x%02x: ", args.addr, reg);
    for (int i = 0; i < n_args; i++)
    {
        cli_print(cli, "0x%02x ", data[i]);
    }

    cli_print(cli, "%s", cli->eol);
}

    /*
     *
     */

static void cmd_i2c_wr(CLI *cli, CliCommand *)
{
    struct CliArgs args = { 0 };

    if (!cli_i2c_parse(cli, & args, true))
    {
        return;
    }

    uint8_t data[16+1];
    uint32_t len = 0;

    for (int i = 0; i < 16; i++)
    {
        if (!args.args[i].valid) break;
        data[i] = args.args[i].value;
        len += 1;
    }

    if (len < 2)
    {
        cli_print(cli, "no wr data%s", cli->eol);
        return;
    }

    if (!args.i2c->write(args.addr, data, len))
    {
        cli_print(cli, "error writing to '0x%02x'%s", args.addr, cli->eol);
        return;
    }

    cli_print(cli, "written, %d bytes%s", (int) len, cli->eol);
}

    /*
     *
     */

static CliCommand cmd_scan = { .cmd="scan", .handler=cmd_i2c_scan, .help="i2c scan [dev] [addr]" };
static CliCommand cmd_rd = { .cmd="rd", .handler=cmd_i2c_rd, .help="i2c rd [dev] <addr> <reg> <n_regs>", .next=& cmd_scan, };
static CliCommand cmd_wr = { .cmd="wr", .handler=cmd_i2c_wr, .help="i2c wr [dev] <addr> <reg> <data> [<data *]", .next=& cmd_rd, };

static CliCommand cmd_i2c = { .cmd="i2c", .handler=0, .subcommand=& cmd_wr };

    /*
     *
     */

void add_cli_i2c_cmd(CLI *cli, I2C* (*fn)(const char *name, void *), void *arg)
{
    lookup_fn = fn;
    lookup_fn_arg = arg;
    cli_insert(cli, & cli->head, & cmd_i2c);
}

}   //  namespace panglos

#endif  //  PO_I2C

//  FIN
