
#include <stdlib.h>
#include <string.h>

#if !defined(ARCH_LINUX)
#include "mdns.h"
#endif

#include "panglos/debug.h"

#include "panglos/socket.h"
#include "panglos/net.h"

namespace panglos {

panglos::List<Network::Interface*> Network::interfaces(Network::Interface::get_next);
panglos::List<Network::Connect*> Network::connects(Network::Connect::get_next);

void Network::add_interface(const char *ip)
{
    interfaces.push(new Interface(strdup(ip)), 0);
}

static int match(Network::Interface* iface, void *arg)
{
    ASSERT(arg);
    const char *ip = (const char *) arg;
    return strcmp(ip, iface->ip) == 0;
}

void Network::del_interface(const char *ip)
{
    Network::Interface* iface = interfaces.find(match, (void*) ip, 0);
    if (iface)
    {
        interfaces.remove(iface, 0);
        free((void*) iface->ip);
        delete iface;
    }
}

void Network::add_connect(Network::Connect *con)
{
    connects.push(con, 0);
}

void Network::del_connect(Network::Connect *con)
{
    connects.remove(con, 0);
}

#if !defined(ARCH_LINUX)

void Network::start_mdns(const char *name)
{
    PO_DEBUG("%s", name);

    esp_err_t err = mdns_init();
    if (err != ESP_OK)
    {
        PO_ERROR("");
        return;
    }

    mdns_hostname_set(name);
}

#else

void Network::start_mdns(const char *name)
{
    PO_ERROR("mDNS not implemented %s", name);
}

#endif  //  ARCH_LINUX

}   //  namespace panglos

//  FIN
