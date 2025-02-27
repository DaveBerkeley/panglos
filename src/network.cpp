
#include <string.h>
#include <stdlib.h>

#include "panglos/debug.h"

#include "panglos/mutex.h"
#include "panglos/semaphore.h"
#include "panglos/list.h"
#include "panglos/batch.h"
#include "panglos/object.h"
#include "panglos/network.h"

    /*
     *
     */

namespace panglos {

    /*
     *  AccessPoint is simply the ssid/pw credentials for known APs
     */

AccessPoint::AccessPoint(const char *_ssid, const char *_pw)
:   next(0),
    ssid(strdup(_ssid)),
    pw(strdup(_pw))
{
}

AccessPoint::~AccessPoint()
{
    free((void*) ssid);
    free((void*) pw);
}

int AccessPoint::match_ssid(AccessPoint *ap, void *arg)
{
    ASSERT(arg);
    const char *ssid = (const char *) arg;
    return strcmp(ssid, ap->ssid) == 0;
}

    /*
     *
     */

static int _connect(Connection *con, void *arg)
{
    Interface *iface = (Interface*) arg;
    con->on_connect(iface);
    return 0;
}

static int _disconnect(Connection *con, void *arg)
{
    Interface *iface = (Interface*) arg;
    con->on_disconnect(iface);
    return 0;
}

    /*
     *
     */

Interface::Interface(const char *_name)
:   next(0),
    name(strdup(_name)),
    con_mutex(0),
    connections(Connection::get_next)
{
    con_mutex = Mutex::create(Mutex::SYSTEM);
}

Interface::~Interface()
{
    free((void*) name);
    delete con_mutex;
}

void Interface::add_connection(Connection *con)
{
    connections.push(con, con_mutex);
}

void Interface::del_connection(Connection *con)
{
    connections.remove(con, con_mutex);
}

void Interface::on_connect()
{
    connections.visit(_connect, this, con_mutex);
}

void Interface::on_disconnect()
{
    connections.visit(_disconnect, this, con_mutex);
}

int Interface::match_name(Interface *iface, void *arg)
{
    ASSERT(arg);
    const char *name = (const char *) arg;
    return strcmp(name, iface->name) == 0;
}

    /*
     *
     */

Network::Network(panglos::Mutex *m)
:   mutex(m),
    interfaces(Interface::get_next)
{
}

Network::~Network()
{
    // unlink the list, but don't delete the Interfaces
    while (interfaces.head)
    {
        interfaces.pop(mutex);
    }
    delete mutex;
}

void Network::add_interface(Interface *iface)
{
    interfaces.append(iface, mutex);
}

Interface *Network::get_interface(const char *name)
{
    if (!name)
    {
        // If no name specified, just return the first interface
        return interfaces.head;
    }
    return interfaces.find(Interface::match_name, (void*) name, mutex);
}

Interface *Network::get_interface(Network::Iterator *iter)
{
    Lock lock(mutex);

    if (!iter->iface)
    {
        // first call (also last if empty)
        iter->iface = interfaces.head;
        return iter->iface;
    }

    if (!interfaces.has(iter->iface, 0))
    {
        // no longer in the list!
        return 0;
    }

    iter->iface = *interfaces.next_fn(iter->iface);
    return iter->iface;
}

    /*
     *
     */

const char *
Interface::IpAddr::tostr(char *buff, size_t s)
{
    return inet_ntop(v4.sin_family, & v4.sin_addr, buff, (socklen_t) s);
}

    /*
     *
     */

WiFiInterface::WiFiInterface(const char *name)
:   Interface(name),
    access_points(AccessPoint::get_next),
    ap_mutex(0)
{
    ap_mutex = panglos::Mutex::create(Mutex::SYSTEM);
}

WiFiInterface::~WiFiInterface()
{
    while (access_points.head)
    {
        AccessPoint *ap = access_points.pop(ap_mutex);
        delete ap;
    }
    delete ap_mutex;
}

void WiFiInterface::add_ap(const char *ssid, const char *pw)
{
    access_points.append(new AccessPoint(ssid, pw), ap_mutex);
}

void WiFiInterface::del_ap(const char *ssid)
{
    Lock lock(ap_mutex);

    AccessPoint *ap = access_points.find(AccessPoint::match_ssid, (void*) ssid, 0);
    if (ap)
    {
        access_points.remove(ap, 0);
        delete ap;
    }
}

AccessPoint *WiFiInterface::get_ap(WiFiInterface::ApIter *iter)
{
    Lock lock(ap_mutex);

    if (iter->ap == 0)
    {
        // First pass
        iter->ap = access_points.head;
        return iter->ap;
    }
    if (access_points.has(iter->ap, 0))
    {
        iter->ap = *access_points.next_fn(iter->ap);
    }

    return iter->ap;
}

    /*
     *
     */

void ConnectionWaiter::on_connect(Interface *i)
{
    PO_DEBUG("");
    iface = i;
    sem->post();
}

void ConnectionWaiter::on_connect_fail(Interface *i)
{
    PO_DEBUG("");
    iface = i;
    sem->post();
}

void ConnectionWaiter::on_disconnect(Interface *)
{
    PO_DEBUG("");
    iface = 0;
    sem->post();
}

ConnectionWaiter::ConnectionWaiter()
:   sem(0)
{
    sem = Semaphore::create();
}

ConnectionWaiter::~ConnectionWaiter()
{
    delete sem;
}

bool ConnectionWaiter::wait()
{
    PO_DEBUG("");
    sem->wait();
    const bool connected = iface ? iface->is_connected() : false;
    PO_DEBUG("%s", connected ? "connected" : "not connected");
    return connected;
}

}   //  namespace panglos

//  FIN

