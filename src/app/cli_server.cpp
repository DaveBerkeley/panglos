
#include <stdlib.h>
#include <string.h>

#include "panglos/debug.h"

#include "cli/src/cli.h"

#include "panglos/mutex.h"
#include "panglos/thread.h"
#include "panglos/network.h"
#include "panglos/cli_net.h"
#include "panglos/object.h"
#include "panglos/storage.h"
#include "panglos/socket.h"
#include "panglos/tx_net.h"
#include "panglos/batch.h"

#include "panglos/app/event.h"
#include "panglos/app/cli_server.h"

namespace panglos {

class NetJob : public BatchTask::Job 
{
    NetJob *next;
public:
    NetJob() : next(0) { }
    static NetJob **get_next(NetJob *nj) { return & nj->next; }
};

    /*
     *
     */

static const char *get_ip(char *ip, size_t s)
{
    Network *net = (Network *) Objects::objects->get("net");
    if (!net)
    {
        PO_ERROR("no network");
        return 0;
    }

    Interface *iface = net->get_interface();    
    if (!iface)
    {
        PO_ERROR("no interface running");
        return 0;
    }

    Interface::State state;

    if (!iface->is_connected(& state))
    {
        PO_ERROR("interface not connected");
        return 0;
    }

    return state.ip.tostr(ip, s);
}

static void server(void *arg)
{
    ASSERT(arg);
    CliCommand *commands = (CliCommand*) arg;

    char ip[32];

    if (!get_ip(ip, sizeof(ip)))
    {
        return;
    }

    struct CliServer cs = {
        .host = ip,
        .port = "6668",
        .commands = commands,
    };

    cli_server(& cs);
}

bool net_cli_init(void *, Event *, Event::Queue *)
{
    PO_DEBUG("");

    CliCommand *commands = (CliCommand*) Objects::objects->get("cli_commands");
 
    Thread *thread = Thread::create("cli_server");
    thread->start(server, commands);

    return false; // INIT handlers must return false so multiple handlers can be run
}

    /*
     *
     */

static void net_server(void *arg)
{
    ASSERT(arg);
    int *port = (int*) arg;

    char ip[32];

    if (!get_ip(ip, sizeof(ip)))
    {
        return;
    }

    Socket *socket = 0;
    {
        char buff[24];
        snprintf(buff, sizeof(buff), "%d", (int) *port);
        socket = Socket::open_tcpip(ip, buff, Socket::SERVER);
    }
 
    TxFactory *factory = TxFactory::create();

    Objects::objects->add("tx_socket", factory->get_socket());

    run_socket_server(socket, factory);
    delete factory;
    delete socket;
    Objects::objects->remove("tx_socket");
}

    /*
     *
     */

bool net_server_init(void *arg, Event *, Event::Queue *)
{
    PO_DEBUG("");
    ASSERT(arg);
    int32_t *port = (int32_t*) arg;

    Thread *thread = Thread::create("server");
    thread->start(net_server, port);

    return false; // INIT handlers must return false so multiple handlers can be run
}

}   //  namespace panglos

//  FIN
