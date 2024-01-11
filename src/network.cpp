
#if defined(ARCH_LINUX)
#include <arpa/inet.h>
#endif

#include <gtest/gtest.h>

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

int Interface::connect(Connection *con, void *arg)
{
    Interface *iface = (Interface*) arg;
    con->on_connect(iface);
    return 0;
}

int Interface::disconnect(Connection *con, void *arg)
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
    mutex(0),
    name(strdup(_name)),
    connections(Connection::get_next)
{
}

Interface::~Interface()
{
    free((void*) name);
}

void Interface::add_connection(Connection *con)
{
    connections.push(con, mutex);
}

void Interface::del_connection(Connection *con)
{
    connections.remove(con, mutex);
}

void Interface::on_connect()
{
    connections.visit(connect, this, mutex);
}

void Interface::on_disconnect()
{
    connections.visit(disconnect, this, mutex);
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
    interfaces.push(iface, mutex);
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
    mutex(0)
{
    mutex = panglos::Mutex::create(Mutex::SYSTEM);
}

WiFiInterface::~WiFiInterface()
{
    while (access_points.head)
    {
        AccessPoint *ap = access_points.pop(mutex);
        delete ap;
    }
    delete mutex;
}

void WiFiInterface::add_ap(const char *ssid, const char *pw)
{
    access_points.append(new AccessPoint(ssid, pw), mutex);
}

void WiFiInterface::del_ap(const char *ssid)
{
    Lock lock(mutex);

    AccessPoint *ap = access_points.find(AccessPoint::match_ssid, (void*) ssid, 0);
    if (ap)
    {
        access_points.remove(ap, 0);
        delete ap;
    }
}

}   //  namespace panglos

//  FIN

