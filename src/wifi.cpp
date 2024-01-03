
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
:   iface(w),
    aps(WiFi::AP::get_next),
    semaphore(0),
    mutex(0),
    connected(false)
{
    PO_DEBUG("");
    ASSERT(iface);
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

    /*
     *  Helper class to manage Semaphore
     */

class Sem {
    Semaphore **sem;
public:
    Sem(Semaphore **s)
    :   sem(s)
    {
        ASSERT(sem);
        ASSERT(!*sem);
        *sem = Semaphore::create();
    }
    ~Sem()
    {
        delete *sem;
        *sem = 0;
    }
};

    /*
     *
     */

bool WiFi::connect()
{
    Lock lock(mutex);

    connected = false;
    Sem sem(& semaphore);

    for (struct AP *ap = aps.head; ap; ap = ap->next)
    {
        iface->connect(this, ap->ssid, ap->pw);
        // wait for connect/disconnect events
        semaphore->wait();

        if (connected)
        {
            return true;
        }
    }

    return false;
}

int WiFi::AP::match(WiFi::AP *ap, void *arg)
{
    // return true if ssid matches
    ASSERT(arg);
    const char *s = (const char *) arg;
    return strcmp(ap->ssid, s) == 0;
}

const char *
WiFiInterface::ip_addr::tostr(char *buff, size_t s)
{
    return inet_ntop(v4.sin_family, & v4.sin_addr, buff, s);
}

void WiFi::on_disconnect(WiFiInterface::Connection *con)
{
    if (con)
    {
        char buff[32];
        con->ip.tostr(buff, sizeof(buff));
        PO_DEBUG("%s", buff);
    }
    else
    {
        PO_DEBUG("");
    }
    connected = false;
    if (semaphore)
    {
        semaphore->post();
    }
}

void WiFi::on_connect(WiFiInterface::Connection *con)
{
    ASSERT(con);
    char buff[32];
    con->ip.tostr(buff, sizeof(buff));
    PO_DEBUG("%s", buff);

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

    Sem sem(& semaphore);

    iface->disconnect(this);
    semaphore->wait();
    return true;
}

}   //  namespace panglos

//  FIN
