
#include <string.h>

#include "esp_sntp.h"
#include "esp_idf_version.h"

#include "panglos/debug.h"
#include "panglos/storage.h"
#include "panglos/mutex.h"
#include "panglos/event_queue.h"
#include "panglos/object.h"
#include "panglos/ntp.h"

#include "cli/src/cli.h"

#include "panglos/app/cli.h"
#include "panglos/app/event.h"

extern "C" void sntp_sync_time(struct timeval *tv)
{
    PO_DEBUG("");
    settimeofday(tv, NULL);
    sntp_set_sync_status(SNTP_SYNC_STATUS_COMPLETED);
}

namespace panglos {

    /*
     *
     */

static bool on_init(void *arg, Event *, Event::Queue *q)
{
    PO_DEBUG("");
    ASSERT(arg);
    Time::tick_t period  = *(Time::tick_t *) arg;

    Storage db("ntp");

    char server[40];
    size_t s = sizeof(server);
    if (!db.get("server", server, & s))
    {
        strncpy(server, "pool.ntp.org", sizeof(server));
    }

    PO_DEBUG("Starting SNTP %s", server);
    const uint32_t period_ms = 1000 * 60 * 60 * 10;
    sntp_set_sync_interval(period_ms);

#if (ESP_IDF_VERSION_MAJOR == 5) && (ESP_IDF_VERSION_MINOR > 0)
    esp_sntp_setoperatingmode(SNTP_OPMODE_POLL);
    esp_sntp_setservername(0, strdup(server));
    esp_sntp_init();
#else
#define n(x) #x
#pragma message(n(ESP_IDF_VERSION_MINOR))
    // Old API
    sntp_setoperatingmode(SNTP_OPMODE_POLL);
    sntp_setservername(0, strdup(server));
    sntp_init();
#endif

    return false; // INIT handlers must return false so multiple handlers can be run
}

    /*
     *
     */

static void cli_ntp(CLI *cli, CliCommand *)
{
    struct timeval tv;
    gettimeofday(& tv, NULL);
    struct tm tm;
    gmtime_r(& tv.tv_sec, & tm);

    cli_print(cli, "%04d/%02d/%02d %02d:%02d:%02d%s",
            tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
            tm.tm_hour, tm.tm_min, tm.tm_sec,
            cli->eol);
}

static CliCommand ntp_cmd = { "ntp", cli_ntp, "show current time",  };

    /*
     *
     */

void ntp_start(Time::tick_t period)
{
    PO_DEBUG("");
    static Time::tick_t p = period;
    EventHandler::add_handler(Event::INIT, on_init, & p);
    add_cli_command(& ntp_cmd);
}

}   //  namespace panglos

//  FIN
