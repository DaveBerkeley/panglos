
#include <string.h>
#include <stdlib.h>

#include <arpa/inet.h>

#include "panglos/debug.h"
#include "panglos/semaphore.h"

#include "panglos/wifi.h"

namespace panglos {

    /*
     *
     */

WiFi::WiFi(WiFiInterface *w)
:   wifi(w),
    aps(WiFi::AP::get_next),
    semaphore(0),
    mutex(0),
    connected(false)
{
    PO_DEBUG("");
    ASSERT(wifi);
    // SYSTEM Mutex as we need the scheduler to run
    mutex = Mutex::create(Mutex::SYSTEM);
}

WiFi::~WiFi()
{
    PO_DEBUG("");

    while (!aps.empty())
    {
        struct AP *ap = aps.pop(mutex);
        free((void*) ap->ssid);
        free((void*) ap->pw);
        delete ap;
    }

    delete mutex;
}

void WiFi::add_ap(const char *ssid, const char *pw)
{
    PO_DEBUG("%s", ssid);
    struct AP *ap = new struct AP;
    ap->ssid = strdup(ssid);
    ap->pw = strdup(pw);
    aps.append(ap, mutex);
}

bool WiFi::connect()
{
    Lock lock(mutex);

    connected = false;
    semaphore = Semaphore::create();

    for (struct AP *ap = aps.head; ap; ap = ap->next)
    {
        wifi->connect(this, ap->ssid, ap->pw);
        // wait for connect/disconnect events
        semaphore->wait();

        if (connected)
        {
            break;
        }
    }

    delete semaphore;
    semaphore = 0;

    return connected;
}

int WiFi::AP::match(WiFi::AP *ap, void *arg)
{
    // return true if ssid matches
    ASSERT(arg);
    const char *s = (const char *) arg;
    return strcmp(ap->ssid, s) == 0;
}

void WiFi::on_disconnect()
{
    PO_DEBUG("");
    connected = false;
    if (semaphore)
    {
        semaphore->post();
    }
}

void WiFi::on_connect(WiFiInterface::Connection *con)
{
    char buff[32];
    PO_DEBUG("%s", inet_ntop(con->ip.v4.sin_family, & con->ip.v4.sin_addr, buff, sizeof(buff)));

    connected = true;
    if (semaphore)
    {
        semaphore->post();
    }
}

bool WiFi::is_connected()
{
    return connected;
}

bool WiFi::disconnect()
{
    Lock lock(mutex);

    if (!is_connected())
    {
        return true;
    }

    semaphore = Semaphore::create();
    wifi->disconnect(this);
    semaphore->wait();
    delete semaphore;
    semaphore = 0;
    return true;
}

}   //  namespace panglos

//  FIN
