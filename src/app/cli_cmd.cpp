
#include <string.h>
#include <math.h>
#include <arpa/inet.h> // for htonl()

#include "cli/src/cli.h"

#include "panglos/debug.h"
#include "panglos/object.h"
#include "panglos/queue.h"
#include "panglos/logger.h"
#include "panglos/mutex.h"
#include "panglos/thread.h"
#include "panglos/time.h"

#include "panglos/mqtt.h"
#include "panglos/storage.h"
#include "panglos/verbose.h"

#include "panglos/drivers/uart.h"
#include "panglos/drivers/gpio.h"
#if defined(PO_PWM)
#include "panglos/drivers/pwm.h"
#endif
#if defined(PO_RTC)
#include "panglos/drivers/rtc.h"
#endif
#if defined(PO_I2C)
#include "panglos/drivers/i2c.h"
#endif
#if defined(PO_SPI)
#include "panglos/drivers/spi.h"
#endif

#include "panglos/app/cli.h"
#include "panglos/app/event.h"
#include "panglos/app/cli_cmd.h"

namespace panglos {

    /*
     *  CLI
     */

void cli_error(CLI *cli, const char *text)
{
    cli_print(cli, "%s%s", text, cli->eol);
}

static bool isin(const char *str, const char **table)
{
    for (; *table; table++)
    {
        if (!strcmp(str, *table)) return true;
    }
    return false;
}

static void cli_print_table(CLI *cli, const char **table, const char *sep="")
{
    for (int i = 0; *table; table++, i++)
    {
        cli_print(cli, "%s%s", (i == 0) ? "" : sep, *table);
    }
}

bool get_args(CLI *cli, int idx, int *args, int n)
{
    ASSERT(args);

    for (int i = 0; i < n; i++)
    {
        const char *s = cli_get_arg(cli, idx);
        if (!s)
        {
            return false;
        }

        if (!cli_parse_int(s, args, 0))
        {
            return false;
        }

        idx += 1;
        args += 1;
    }
    return true;
}

    /*
     *
     */

static void cmd_reset(CLI *cli, CliCommand *cmd)
{
    IGNORE(cli);
    IGNORE(cmd);
    ASSERT(0);
}

    /*
     *
     */

#if defined(PO_PWM)

static void cmd_pwm(CLI *cli, CliCommand *)
{
    PWM *pwm = (PWM*) Objects::objects->get("pwm");
    ASSERT(pwm);

    int args[2];

    if (!get_args(cli, 0, args, 2))
    {
        cli_error(cli, "expecting : pwm <idx> <value>");
        return;
    }

    bool ok = pwm->set(args[0], (unsigned) args[1]);

    if (!ok)
    {
        cli_print(cli, "error setting chan=%d value=%d%s", args[0], args[1], cli->eol);
    }
}

#endif // PO_PWM

    /*
     *
     */

static int match_name(CliCommand *cmd, void *arg)
{
    ASSERT(arg);
    const char *s = (const char*) arg;
    return strcmp(cmd->cmd, s) == 0;
}

static void print_help(CLI *cli, CliCommand *cmd)
{
    ASSERT(cli);
    ASSERT(cmd);
    cli_print(cli, "%s : %s%s", cmd->cmd, cmd->help, cli->eol);
}

static void print_help_nest(CLI *cli, CliCommand *cmd, int nest)
{
    for (; cmd; cmd = cmd->next)
    {
        cli_print(cli, "%*s", nest, "");
        print_help(cli, cmd);
        if (cmd->subcommand)
        {
            print_help_nest(cli, cmd->subcommand, nest + 1);
        }
    }
}

static void cmd_help(CLI *cli, CliCommand *cmd)
{
    IGNORE(cmd);
    const char *s = cli_get_arg(cli, 0);

    if (s)
    {
        cmd = cli_find(& cli->head, match_name, (void*) s);
        if (!cmd)
        {
            cli_print(cli, "command not found%s", cli->eol);
            return;
        }

        print_help(cli, cmd);
        return;
    }

    print_help_nest(cli, cli->head, 0);
}

#if 0 // !defined(ARCH_LINUX)

    /*
     *  This should be in FreeRTOS specific panglos code
     */

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static LUT lut_task_state[] = {
    {   "Ready", 	    eReady, },
    {   "Running", 	    eRunning, },
    {   "Blocked", 	    eBlocked, },
    {   "Suspended",    eSuspended, },
    {   "Deleted", 	    eDeleted, },
    {   0, 0 }
};

static void cmd_thread(CLI *cli, CliCommand *)
{
    const UBaseType_t num_tasks = uxTaskGetNumberOfTasks();
    TaskStatus_t *tasks = (TaskStatus_t*) malloc(sizeof(TaskStatus_t) * num_tasks);

    const UBaseType_t n = uxTaskGetSystemState(tasks, num_tasks, 0);
    ASSERT(n == num_tasks);

    const int cols[] = { 3, 20, 10, 10 };
    cli_print(cli, "%*s %-*s %-*s %-*s%s", 
            cols[0], "#", 
            cols[1], "Task", 
            cols[2], "State", 
            cols[3], "Stack_HWM", 
            cli->eol);
 
    for (UBaseType_t i = 0; i < num_tasks; ++i)
    {
        TaskStatus_t *task = & tasks[i];

        cli_print(cli, "%*d %-*s %-*s %-*ld%s", 
            cols[0], (int) task->xTaskNumber, 
            cols[1], task->pcTaskName, 
            cols[2], lut(lut_task_state, task->eCurrentState),
            cols[3], (long) tasks[i].usStackHighWaterMark, 
            cli->eol);
    }

    free(tasks);
}

#endif  //  ARCH_LINUX

    /*
     *
     */

static void cmd_uart(CLI *cli, CliCommand *cmd)
{
    IGNORE(cmd);
    UART *uart = (UART*) Objects::objects->get("uart");
    ASSERT(uart);

    for (int i = 0; i < CLI_MAX_ARGS; i++)
    {
        const char *s = cli_get_arg(cli, i);
        if (!s)
        {
            break;
        }

        uart->tx(s, int(strlen(s)));
        uart->tx(" ", 1);
    }

    uart->tx("\r\n", 2);
}

    /*
     *
     */

static void gpio_flash(CLI *cli, int idx)
{
    int delay = 500; //ms
    const char *s = cli_get_arg(cli, idx);
    if (cli_parse_int(s, & delay, 0))
    {
        cli_print(cli, "flash rate=%d ms%s", delay, cli->eol);
        idx += 1;
    }
    while (true)
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

static void cmd_gpio(CLI *cli, CliCommand *cmd)
{
    IGNORE(cmd);
    bool toggle = false;
    bool show = false;
    int idx = 0;
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
    }

    if (s && !strcmp(s, "flash"))
    {
        gpio_flash(cli, idx);
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

    // if "gpio show <name>" loop forever printing value
    while (show)
    {
        cli_print(cli, "%s %d%s", name, gpio->get(), cli->eol);
        Time::msleep(500);
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

#if !defined(GTEST)

static LUT st_lut[] = { 
    {   "NONE",     Storage::VAL_NONE,  },
    {   "INT8",     Storage::VAL_INT8,  }, 
    {   "INT16",    Storage::VAL_INT16, },
    {   "INT32",    Storage::VAL_INT32, },
    {   "STR",      Storage::VAL_STR,   },
    {   "BLOB",     Storage::VAL_BLOB,  },
    {   "OTHER",    Storage::VAL_OTHER, },
    { 0, 0 },
};

static void cmd_storage_list(CLI *cli, CliCommand *)
{
    int idx = 0;
    char *s = (char*) cli_get_arg(cli, idx++);

    Storage::List list(s);

    char ns[24];
    char key[24];
    Storage::Type type;

    while (list.get(ns, key, & type))
    {
        cli_print(cli, "%s %s %s%s", ns, key, lut(st_lut, type), cli->eol);
    }
}

static void cmd_storage_set(CLI *cli, CliCommand *)
{
    int idx = 0;
    const char *ns  = cli_get_arg(cli, idx++);
    const char *key = cli_get_arg(cli, idx++);
    const char *val = cli_get_arg(cli, idx++);

    if (!(ns || key || val))
    {
        cli_print(cli, "expects: ns key value%s", cli->eol);
        return;
    }

    Storage db(ns);

    bool is_string = false;

    int value = 0;
    if (!cli_parse_int(val, & value, 0))
    {
        is_string = true;
    }

    // Infer type from the value
    if (val[0] == '\'')
    {
        is_string = true;
        // remove the "'" chars
        val += 1;
        char *end = (char*) strchr(val, '\'');
        if (!end)
        {
            cli_print(cli, "no terminating \"'\" in value%s", cli->eol);
            return;
        }
        *end = '\0';
    }

    bool ok = false;
    if (is_string)
    {
        ok = db.set(key, val);
    }
    else
    {
        ok = db.set(key, (int32_t) value);
    }

    if (!ok)
    {
        cli_print(cli, "Error writing %s %s %s%s", ns, key, val, cli->eol);
    }
    else
    {
        db.commit();
    }
}

static void cmd_storage_get(CLI *cli, CliCommand *cmd)
{
    IGNORE(cmd);
    int idx = 0;
    const char *ns  = cli_get_arg(cli, idx++);
    const char *key = cli_get_arg(cli, idx++);

    if (!(ns || key))
    {
        cli_print(cli, "expects: ns key%s", cli->eol);
        return;
    }

    Storage db(ns, false);

    Storage::Type type = db.get_type(key);
    bool ok = false;

    switch (type)
    {
        case Storage::VAL_INT8  :
        {
            int8_t value;
            ok = db.get(key, & value);
            cli_print(cli, "%#02x%s", value, cli->eol);
            break;
        }
        case Storage::VAL_INT16 :
        {
            int16_t value;
            ok = db.get(key, & value);
            cli_print(cli, "%#04x%s", value, cli->eol);
            break;
        }
        case Storage::VAL_INT32 :
        {
            int32_t value;
            ok = db.get(key, & value);
            cli_print(cli, "%#08x%s", (int) value, cli->eol);
            break;
        }
        case Storage::VAL_STR   :
        {
            char value[64];
            size_t s = sizeof(value);
            ok = db.get(key, value, & s);
            if (!strcmp(key, "pw"))
            {
                cli_print(cli, "%s%s", "********", cli->eol);
            }
            else
            {
                cli_print(cli, "%s%s", value, cli->eol);
            }
            break;
        }
        default :
        {
            cli_print(cli, "Unknown type=%d%s", type, cli->eol);
            break;
        }
    }

    if (!ok)
    {
        cli_print(cli, "Error reading %s%s", key, cli->eol);
    }
}

static void dump(CLI *cli, const void *data, size_t size, int columns=16)
{
    const char *d = (char*) data;
    int addr = 0;
    while (size)
    {
        cli_print(cli, "%04d:", addr);
        addr += columns;
        for (int i = 0; i < columns; i++)
        {
            if (!size) break;
            uint8_t c = *d++;
            cli_print(cli, " %x%x", c >> 4, c & 0x0f);
            size -= 1;
        }
        cli_print(cli, "%s", cli->eol);
    }
}

static void blob_get(CLI *cli, const char *ns, const char *key)
{
    const bool verbose = true;
    Storage db(ns, verbose);

    size_t size = 0;

    if (!db.get_blob(key, 0, & size))
    {
        cli_print(cli, "error reading '%s':'%s'%s", ns, key, cli->eol);
        return;
    }
    if (!size)
    {
        cli_print(cli, "error size==0%s", cli->eol);
        return;
    }

    void *data = malloc(size);
    ASSERT(data);
    if (!db.get_blob(key, data, & size))
    {
        free(data);
        cli_print(cli, "error getting '%s':'%s'%s", ns, key, cli->eol);
        return;
    }

    dump(cli, data, size);
    free(data);
}

    /*
     *
     */

class Capture
{
    CLI *cli;
    char last;
    size_t chunk;
    size_t size;
public:
    char *ns;
    char *key;
    char *data;
    size_t idx;

    Capture(CLI *_cli, const char *_ns, const char *_key, size_t init_size=128)
    :   cli(_cli), 
        last(0),
        chunk(init_size),
        size(init_size),
        ns(strdup(_ns)),
        key(strdup(_key)),
        data(0),
        idx(0)
    {
        data = (char*) malloc(size);
        ASSERT(data);
    }

    ~Capture()
    {
        free(data);
        free(ns);
        free(key);
    }

    bool process(char c)
    {
        // save c in data
        data[idx++] = c;
        if (idx == size)
        {
            // we've run out of space, so realloc
            size += chunk;
            data = (char*) realloc(data, size);
            ASSERT(data); 
        }
        data[idx] = '\0';

        const bool done = (c == '\n') && (last == '\n');
        last = c;
        if (done) PO_DEBUG("");
        return !done;
    }
};

static void captured(CLI *cli, char c)
{
    Capture *cap = (Capture *) cli->capture_ctx;
    ASSERT(cap);
    if (cap->process(c))
    {
        return;
    }
    // on captured ...

    Storage db(cap->ns);

    if (!db.set_blob(cap->key, cap->data, cap->idx))
    {
        cli_print(cli, "error writing blob '%s':'%s' size=%d%s", 
                cap->ns,
                cap->key, 
                cap->idx,
                cli->eol);
    }
    else
    {
        cli_print(cli, "set blob '%s':'%s' size=%d%s", 
                cap->ns,
                cap->key, 
                cap->idx,
                cli->eol);
    }

    // end capture
    cli_capture(cli, 0, 0);
    delete cap;
}

static void blob_set(CLI *cli, const char *ns, const char *key)
{
    const bool verbose = true;
    Storage db(ns, verbose);
    cli_capture(cli, captured, new Capture(cli, ns, key));
}

static void cmd_storage_blob(CLI *cli, CliCommand *)
{
    int idx = 0;
    const char *cmd  = cli_get_arg(cli, idx++);
    const char *ns  = cli_get_arg(cli, idx++);
    const char *key = cli_get_arg(cli, idx++);

    const char *cmds[] = { "set", "get", 0 };

    if ((!(ns && key && cmd)) || !isin(cmd, cmds))
    {
        cli_print(cli, "expects: ");
        cli_print_table(cli, cmds, "|");
        cli_print(cli, " ns key%s", cli->eol);
        return;
    }

    if (!strcmp(cmd, "get"))
    {
        blob_get(cli, ns, key);
        return;
    }

    blob_set(cli, ns, key);
}

static void cmd_storage_del(CLI *cli, CliCommand *cmd)
{
    IGNORE(cmd);
    int idx = 0;
    const char *ns  = cli_get_arg(cli, idx++);
    const char *key = cli_get_arg(cli, idx++);

    if (!(ns || key))
    {
        cli_print(cli, "expects: ns key%s", cli->eol);
        return;
    }

    Storage db(ns);

    const bool ok = db.erase(key);

    if (ok)
    {
        db.commit();
    }
    else
    {
        cli_print(cli, "Error deleting %s%s", key, cli->eol);
    }
}

static CliCommand st_list = { "list", cmd_storage_list, "[namespace]", 0, 0, 0 };
static CliCommand st_set  = { "set",  cmd_storage_set,  "<namespace> <key> <value>", 0, 0, & st_list, };
static CliCommand st_get  = { "get",  cmd_storage_get,  "<namespace> <key>", 0, 0, & st_set, };
static CliCommand st_blob = { "blob", cmd_storage_blob, "set|get <namespace> <key>", 0, 0, & st_get, };
static CliCommand st_del  = { "del",  cmd_storage_del,  "<namespace> <key>", 0, 0, & st_blob, };

#endif  //  GTEST

    /*
     *
     */

#if defined(PO_RTC)

static void cmd_rtc_get(CLI *cli, CliCommand *cmd)
{
    IGNORE(cmd);
    RTC *rtc = (RTC*) Objects::objects->get("rtc");
    ASSERT(rtc);

    RTC::DateTime dt;

    if (!rtc->get(& dt))
    {
        cli_print(cli, "Error reading RTC%s", cli->eol);
        return;
    }

    cli_print(cli, "%04d-%02d-%02d %02d:%02d:%02d%s", dt.year, dt.month, dt.day, dt.hour, dt.min, dt.sec, cli->eol);
}

static void cmd_rtc_set(CLI *cli, CliCommand *cmd)
{
    IGNORE(cmd);
    RTC *rtc = (RTC*) Objects::objects->get("rtc");
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
        .sec   = (uint8_t) args[5],
        .min   = (uint8_t) args[4],
        .hour  = (uint8_t) args[3],
        .day   = (uint8_t) args[2],
        .month = (uint8_t) args[1],
        .year  = (uint16_t) args[0],
    };

    //cli_print(cli, "%04d-%02d-%02d %02d:%02d:%02d%s", dt.year, dt.month, dt.day, dt.hour, dt.min, dt.sec, cli->eol);
    if (!rtc->set(& dt))
    {
        cli_print(cli, "Error writing RTC%s", cli->eol);
        return;
    }
}

static CliCommand rtc_set  = { "set",  cmd_rtc_set,  "y m d h m s", 0, 0, 0, };
static CliCommand rtc_get  = { "get",  cmd_rtc_get,  "",  0, 0, & rtc_set, };

#endif // (PO_RTC)

    /*
     *
     */

static void cmd_verbose(CLI *cli, CliCommand *cmd)
{
    IGNORE(cmd);
    int idx = 0;
    const char *s = cli_get_arg(cli, idx++);

    if (s &&!strcmp(s, "list"))
    {
        for (struct Verbose *v = Verbose::verboses; v; v = v->next)
        {
            cli_print(cli, "%s %d%s", v->name, v->verbose, cli->eol);
        }
        return;
    }

    const char *t = cli_get_arg(cli, idx++);

    if (!(s || t))
    {
        cli_print(cli, "expects 'list' or '<name> 0|1'%s", cli->eol);
        return;
    }

    int state = 0;
    if (!cli_parse_int(t, & state, 0))
    {
        cli_print(cli, "can't set verbose %s to state %s%s", s, t, cli->eol);
        return;
    }

    for (struct Verbose *v = Verbose::verboses; v; v = v->next)
    {
        if (!strcmp(s, v->name))
        {
            v->verbose = state != 0;
            cli_print(cli, "%s %d%s", v->name, v->verbose, cli->eol);
            return;
        }
    }    

    cli_print(cli, "%s not found%s", s, cli->eol);
}

static void cmd_banner(CLI *cli, CliCommand *cmd)
{
    IGNORE(cmd);

    const char *banner = (const char*) Objects::objects->get("banner");
    if (!banner)
    {
        return;
    }

    cli_print(cli, "%s%s", banner, cli->eol);
}

    /*
     *
     */

static void dev_visit(const char *name, void *obj, void *arg)
{
    ASSERT(arg);
    CLI *cli = (CLI*) arg;
    cli_print(cli, "%p %s%s", obj, name, cli->eol);
}

static void cmd_devices(CLI *cli, CliCommand *cmd)
{
    IGNORE(cmd);
    Objects::visit(Objects::objects, dev_visit, cli);
}

    /*
     *
     */

#if defined(PO_I2C)

static void cmd_i2c(CLI *cli, CliCommand *)
{
    I2C *i2c = (I2C*) Objects::objects->get("i2c");
    if (!i2c)
    {
        cli_print(cli, "no i2c device%s", cli->eol);
        return;
    }

    const char *s = cli_get_arg(cli, 0);

    if (s)
    {
        int addr = 0;

        if (cli_parse_int(s, & addr, 0))
        {
            const bool ok = i2c->probe((uint8_t) addr, 2);
            if (ok)
            {
                cli_print(cli, "found %#x%s", addr, cli->eol);
            }
            else
            {
                cli_print(cli, "no device found%s", cli->eol);
            }
            return;
        }
    }

    bool found = false;

    for (uint8_t addr = 0; addr < 0x80; addr++)
    {
        const bool ok = i2c->probe(addr, 2);
        if (ok)
        {
            cli_print(cli, "found %#x%s", addr, cli->eol);
            found = true;
        }
    }

    if (!found)
    {
        cli_print(cli, "no device found%s", cli->eol);
    }
}

#endif // (PO_I2C)

    /*
     *
     */

#if defined(PO_SPI)

static void cmd_spi(CLI *cli, CliCommand *)
{
    SpiDevice *spi = (SpiDevice*) Objects::objects->get("spi_dev");
    if (!spi)
    {
        cli_print(cli, "no spi_dev device%s", cli->eol);
        return;
    }

    uint32_t packet[32];
    int words = 0;

    for (int idx = 0; idx < 32; idx++)
    {
        const char *s = cli_get_arg(cli, idx);
        if (!s)
        {
            break;
        }
        long value = 0;

        if (!cli_parse_long(s, & value, 0))
        {
            cli_print(cli, "Error parsing value '%s'%s", s, cli->eol);
            return;
        }

        packet[words++] = htonl(uint32_t(value));
    }

    if (!spi->write((uint8_t*) packet, sizeof(uint32_t) * words))
    {
        cli_print(cli, "Error writing to SPI interface%s", cli->eol);
        return;
    }
}

#endif // PO_SPI)


    /*
     *
     */

#if 0 // defined(ESP_PLATFORM) // TODO : move into network code?

#include "panglos/network.h"

    /*
     *
     */

static void cmd_wifi(CLI *cli, CliCommand *)
{
    cli_print(cli, "wifi%s", cli->eol);

    Network *net = (Network*) Objects::objects->get("net");

    if (!net)
    {
        cli_print(cli, "no network%s", cli->eol);
        return;
    }

    Network::Iterator iter;
    Interface *iface = net->get_interface(& iter);
    if (!iface)
    {
        cli_print(cli, "no interface%s", cli->eol);
        return;
    }

    WiFiInterface *wifi = iface->get_wifi();
    if (!wifi)
    {
        cli_print(cli, "no wifi interface%s", cli->eol);
        return;
    }

    int idx = 0;
    const char *s = cli_get_arg(cli, idx++);
    const char *p = cli_get_arg(cli, idx++);

    if (s && !strcmp(s, "reset"))
    {
        cli_print(cli, "del APs%s", cli->eol);
        struct WiFiInterface::ApIter iter;
        while (true)
        {
            AccessPoint *ap = wifi->get_ap(& iter);
            if (!ap) break;
            wifi->del_ap(ap->ssid);
        }
        return;
    }

    // ssid, pw
    if (s && p)
    {
        cli_print(cli, "adding %s:%s%s", s, p, cli->eol);
        wifi->add_ap(s, p);
    }

    if (wifi->is_connected())
    {
        wifi->disconnect();
    }

    wifi->connect();
}

    /*
     * 
     */

#include <inttypes.h>
#include "panglos/drivers/timer.h"
#include "panglos/esp32/timer.h"

struct TimerCtx
{
    Timer *timer;
    GPIO *led;
    bool periodic;
    Event::Type code;
    Event::Queue *queue;
};

static void on_timer_irq(Timer *, void *arg)
{
    // Called in timer interrupt
    ASSERT(arg);
    struct TimerCtx *ctx = (struct TimerCtx *) arg;
    Event event;
    event.type = ctx->code;
    event.put(ctx->queue);
}

static bool on_event(void *arg, Event *ev, Event::Queue *queue)
{
    // Called in CLI main event loop
    ASSERT(arg);
    struct TimerCtx *ctx = (struct TimerCtx *) arg;
    uint64_t t = ctx->timer->get();
    PO_DEBUG("%#" PRIx64, t);
    return true;
}

static bool cli_parse_64(const char *s, uint64_t *value, int base=0)
{
    char *end = 0;
    long long int val = strtoll(s, & end, base);
    if (!end)
    {
        return false;
    }

    if ('\0' == *end)
    {
        *value = val;
        return true;
    }
    return false;
}

static void cmd_timer(CLI *cli, CliCommand *)
{
    static struct TimerCtx ctx = { .timer=0, .led=0, .periodic=true };
    uint64_t period = 0x1000000L;
    bool set = false;
    bool add = false;
    int idx = 0;
    const char *s = cli_get_arg(cli, idx++);

    if (s && !strcmp(s, "one"))
    {
        ctx.periodic = false;
        s = cli_get_arg(cli, idx++);
    }

    if (s && !strcmp(s, "add"))
    {
        add = true;
        s = cli_get_arg(cli, idx++);
        cli_print(cli, "add%s", cli->eol);
    }

    uint64_t var = 0;
    if (s && cli_parse_64(s, & var, 0))
    {
        period = var;
        set = true;
        s = cli_get_arg(cli, idx++);
        cli_print(cli, "period %#" PRIx64 "%s", period, cli->eol);
    }
 
    if (!ctx.code)
    {
        ctx.code = EventHandler::get_user_code();
        EventHandler::add_handler(ctx.code, on_event, & ctx);
        ctx.queue = (Event::Queue*) Objects::objects->get("cli_queue");
        ctx.led = (GPIO*) Objects::objects->get("led");
    }

    if (!ctx.timer)
    {
        ctx.timer = create_hr_timer(1, 0);
        ctx.timer->set_handler(on_timer_irq, & ctx);
        ctx.timer->set_period(period);
        ctx.timer->start(ctx.periodic);
        cli_print(cli, "start %s%s", ctx.periodic ? "periodic" : "", cli->eol);
    }
    else if (add || set)
    {
        uint64_t p = period;
        if (add)
        {
            uint64_t t = ctx.timer->get();
            cli_print(cli, "timer add %#" PRIx64 "%s", t, cli->eol);
            p += t;
        }
        cli_print(cli, "timer set %#" PRIx64 "%s", p, cli->eol);
        ctx.timer->set_period(p);
    }

    bool loop = s ? (strcmp(s, "loop") == 0) : false;

    if (s && !loop)
    {
        if (strcmp(s, "stop") == 0)
        {
            delete ctx.timer;
            ctx.timer = 0;
            cli_print(cli, "timer deleted%s", cli->eol);
            return;
        }
    }

    do {
        uint64_t t = ctx.timer->get();
        cli_print(cli, "%#" PRIx64 "%s", t, cli->eol);
        Time::msleep(500);
    }   while (ctx.timer && loop);
}

#endif // defined(ESP_PLATFORM)


    /*
     *
     */

static CliCommand cli_commands[] = {
    { "reset",  cmd_reset,  "ASSERT(0)", 0, 0, 0 },
    { "help",   cmd_help,   "help <cmd>", 0, 0, 0 },
#if 0 // !defined(ARCH_LINUX)
    { "threads", cmd_thread, "view threads", 0, 0, 0 },
#endif
    { "gpio",   cmd_gpio,   "gpio", 0, 0, 0 },
    { "verbose", cmd_verbose, "verbose", 0, 0, 0 },
    { "banner", cmd_banner,   "banner", 0, 0, 0 },
    { "db",     cli_nowt,     "list|del|get|set|blob", & st_del, 0, 0 },
#if 0 // defined(ESP_PLATFORM)
    { "wifi", cmd_wifi,   "wifi", 0, 0, 0 },
    { "timer", cmd_timer,   "timer", 0, 0, 0 },
#endif
    { 0, 0, 0, 0, 0, 0 },
};

void add_cli_commands(CLI *cli)
{
    for (size_t i = 0; cli_commands[i].cmd; i++)
    {
        cli_insert(cli, & cli->head, & cli_commands[i]);
    }

#if defined(PO_PWM)
    if (Objects::objects->get("pwm"))
    {
        static CliCommand cmd = { "pwm", cmd_pwm, "<chan> <value>", 0, 0, 0 };
        cli_insert(cli, & cli->head, & cmd);
    }
#endif

    if(Objects::objects->get("uart"))
    {
        static CliCommand cmd = { "uart", cmd_uart, "uart", 0, 0, 0 };
        cli_insert(cli, & cli->head, & cmd);
    }

#if defined(PO_RTC)
    if(Objects::objects->get("rtc"))
    {
        static CliCommand cmd = { "rtc", 0, "", & rtc_get, 0, 0 };
        cli_insert(cli, & cli->head, & cmd);
    }
#endif

#if defined(PO_I2C)
    if(Objects::objects->get("i2c"))
    {
        static CliCommand cmd = { "i2c", cmd_i2c, "i2c", 0, 0, 0 };
        cli_insert(cli, & cli->head, & cmd);
    }
#endif

#if defined(PO_SPI)
    if(Objects::objects->get("spi_dev"))
    {
        static CliCommand cmd = { "spi", cmd_spi, "spi", 0, 0, 0 };
        cli_insert(cli, & cli->head, & cmd);
    }
#endif

    {
        static CliCommand cmd = { "devices", cmd_devices, "list devices", 0, 0, 0 };
        cli_insert(cli, & cli->head, & cmd);
    }
}

}   //  namespace panglos

//  FIN
