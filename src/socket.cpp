
    /*
     *
     */

#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>

#if defined(ARCH_LINUX)
#include <sys/socket.h>
#include <netdb.h>
#else
#include "lwip/sockets.h"
#include "lwip/netdb.h"
#endif

#include "panglos/debug.h"

#include "panglos/thread.h"
#include "panglos/list.h"
#include "panglos/time.h"
#include "panglos/socket.h"
#include "panglos/net.h"

namespace panglos {

    /*
     *
     */

class _Socket : public Socket
{
public:
    int sock;

    virtual int send(const uint8_t *data, size_t len) override
    {
        return (int) ::send(sock, data, len, 0);
    }

    virtual int recv(uint8_t *data, size_t len) override
    {
        return (int) ::recv(sock, data, len, 0);
    }

    static int open(const char *host, const char *port, Role role, bool udp)
    {
        PO_DEBUG("%s:%s", host, port);

        struct addrinfo *addr = 0;
        int err = getaddrinfo(host, port, 0, & addr);
        if (err != 0)
        {
            PO_ERROR("err=%d %s", errno, strerror(errno));
            return -1;
        }

        ASSERT(addr);

        int fd = socket(AF_INET, udp ? SOCK_DGRAM : SOCK_STREAM, 0);
        if (fd < 0)
        {
            PO_ERROR("err=%d %s", errno, strerror(errno));
            return -1;
        }

        int option = 1;
        setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, & option, sizeof(option));

        // Use the first ip address in the list
        switch (role)
        {
            case CLIENT :
            {
                err = ::connect(fd, addr->ai_addr, addr->ai_addrlen);
                break;
            }
            case SERVER :
            {
                err = ::bind(fd, addr->ai_addr, addr->ai_addrlen);
                break;
            }
            default :
                ASSERT(0);
        }

        freeaddrinfo(addr);

        if (err != 0)
        {
            PO_ERROR("err=%d %s", errno, strerror(errno));
            return -1;
        }

        return fd;
    }    

    _Socket(int s) : sock(s) { }

    ~_Socket()
    {
        close(sock);
    }

    static _Socket *create(const char *ip, const char *port, Role role, bool udp)
    {
        int sock = open(ip, port, role, udp);
        return (sock < 0) ? 0 : new _Socket(sock);
    }
};

Socket *Socket::open_udp(const char *ip, const char *port, Role role)
{
    return _Socket::create(ip, port, role, true);
}

Socket *Socket::open_tcpip(const char *ip, const char *port, Role role)
{
    return _Socket::create(ip, port, role, false);
}

    /*
     *
     */

Client::Client(SocketServer *s)
:   thread(0),
    sock(0),
    ss(s),
    name(0),
    next(0)
{
    PO_DEBUG("%p", this);
}

Client::~Client()
{
    PO_DEBUG("%p", this);
    delete sock;
    sock = 0;
    thread->join();
    delete thread;
    free(name);
}

void Client::start(Socket *s)
{
    PO_DEBUG("");
    sock = s;

    const int fd = ((_Socket*) s)->sock;
    char buff[32];
    snprintf(buff, sizeof(buff), "client_%d", fd);
    name = strdup(buff);

    thread = Thread::create(name);
    thread->start(runner, this);
}

void Client::stop()
{
    PO_DEBUG("%p", this);
    delete sock;
    sock = 0;
}

void Client::runner()
{
    PO_DEBUG("start %p", this);
    ss->add_client(this);
    run();
    PO_DEBUG("end");
    ss->del_client(this);
}

void Client::runner(void *arg)
{
    ASSERT(arg);
    Client *client = (Client*) arg;
    client->runner();
}

Client **Client::get_next(Client *c) { return & c->next; }

    /*
     *
     */

class _SocketServer : public SocketServer
{
    int sock;
    Client::Factory *cf;
    typedef List<Client*> Clients;

    Clients pending;
    Clients clients;
    Clients ex_clients;
    Mutex *mutex;
    bool dead;

public:

    class Connect : public Network::Connect
    {
        virtual void on_connect() override
        {
            PO_DEBUG("");
        }
        virtual void on_disconnect() override
        {
            PO_DEBUG("");
        }
    };

    Connect connect;

    _SocketServer(int s, Client::Factory *_cf)
    :   sock(s),
        cf(_cf),
        pending(Client::get_next),
        clients(Client::get_next),
        ex_clients(Client::get_next),
        mutex(0),
        dead(false)
    {
        PO_DEBUG("");
        ASSERT(cf);
        mutex = Mutex::create();
        Network::add_connect(& connect);
    }

    ~_SocketServer()
    {
        PO_DEBUG("");
        Network::del_connect(& connect);
        kill();
        tidy_clients(clients);
        tidy_clients(ex_clients);
        delete mutex;
    }

    virtual void kill() override
    {
        PO_DEBUG("");
        dead = true;

        // wait until all the pending Clients have been started
        while (pending.head)
        {
            Time::msleep(10);
        }
    }

    virtual void add_client(Client *client) override
    {
        PO_DEBUG("%p", client);
        // Move to client list
        pending.remove(client, mutex);
        clients.push(client, mutex);
    }

    virtual void del_client(Client *client) override
    {
        PO_DEBUG("%p", client);
        // Move to ex client list
        clients.remove(client, mutex);
        client->stop();
        ex_clients.push(client, mutex);
    }

    void tidy_clients(Clients &clients)
    {
        while (clients.head)
        {
            Client *client = clients.pop(mutex);
            delete client;
        }
    }

    void run()
    {
        PO_DEBUG("");
        listen(sock, 10);

        while (!dead)
        {
            struct sockaddr addr;
            socklen_t size = sizeof(addr);
            const int fd = accept(sock, & addr, & size);
            if (fd < 0)
            {
                PO_ERROR("err=%s", strerror(errno));
                switch (errno)
                {
                    case EBADF : 
                        {
                            PO_INFO("socket closed");
                            dead = true;
                            continue;
                        }
                    default : break;
                }
                ASSERT(0); // TODO : handle other errors?
            }

            // Handle the connection
            Client *client = cf->create_client(this);
            pending.push(client, mutex);
            client->start(new _Socket(fd));
            tidy_clients(ex_clients);
        }

        // delete all the clients
        tidy_clients(clients);
        tidy_clients(ex_clients);
    }
};

    /*
     *
     */

void run_socket_server(Socket *sock, Client::Factory *cf)
{
    _Socket *s = (_Socket*) sock;
    _SocketServer ss(s->sock, cf);
    ss.run();
    ss.kill();
}

}   //  namespace panglos

//  FIN
