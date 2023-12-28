
#include <stdlib.h>

#if !defined(ARCH_LINUX)
#include "lwip/apps/mdns.h"
#endif

#include "panglos/debug.h"

#include "panglos/socket.h"
#include "panglos/net.h"

namespace panglos {

panglos::List<Network::Interface*> Network::interfaces(Network::Interface::get_next);
panglos::List<Network::Connect*> Network::connects(Network::Connect::get_next);

void Network::add_interface(const char *ip)
{
    interfaces.push(new Interface(ip), 0);
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

    PO_ERROR("NOT IMPLEMENTED");
#if 0
    esp_err_t err = mdns_init();
    if (err != ESP_OK)
    {
        PO_ERROR("");
        return;
    }

    mdns_hostname_set(name);
#endif
}

#else

void Network::start_mdns(const char *name)
{
    PO_ERROR("mDNS not implemented %s", name);
}

#endif  //  ARCH_LINUX

}   //  namespace panglos

//  FIN
