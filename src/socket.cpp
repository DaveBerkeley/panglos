
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
#include "panglos/semaphore.h"
#include "panglos/list.h"
#include "panglos/socket.h"
#include "panglos/network.h"

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
    sem(0),
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
    delete thread;
    free(name);
}

void Client::kill()
{
    ASSERT(thread != Thread::get_current());

    delete sock;
    sock = 0;

    ASSERT(thread);
    thread->join();
}

void Client::start(Socket *s, Semaphore *_sem)
{
    PO_DEBUG("%p", this);
    sock = s;
    sem = _sem;

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
    Socket *s = sock;
    sock = 0;
    delete s;
}

void Client::runner()
{
    PO_DEBUG("start %p", this);
    ASSERT(sem);
    ss->add_client(this);
    sem->post(); // let the server know that we have started
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

    Clients clients;
    Clients ex_clients;
    Mutex *mutex;
    bool dead;

public:

    class Connect : public Connection
    {
        virtual void on_connect(Interface *) override
        {
            PO_DEBUG("");
        }
        virtual void on_connect_fail(Interface *) override
        {
            PO_DEBUG("");
        }
        virtual void on_disconnect(Interface *) override
        {
            PO_DEBUG("");
        }
    };

    Connect connect;

    _SocketServer(int s, Client::Factory *_cf)
    :   sock(s),
        cf(_cf),
        clients(Client::get_next),
        ex_clients(Client::get_next),
        mutex(0),
        dead(false)
    {
        PO_DEBUG("");
        ASSERT(cf);
        mutex = Mutex::create();
        // TODO
        //Network::add_connect(& connect);
    }

    ~_SocketServer()
    {
        PO_DEBUG("");
        // TODO
        //Network::del_connect(& connect);
        delete mutex;
    }

    virtual void kill() override
    {
        PO_DEBUG("");
        dead = true;
    }

    virtual void add_client(Client *client) override
    {
        PO_DEBUG("%p", client);
        //clients.push(client, mutex);
    }

    virtual void del_client(Client *client) override
    {
        PO_DEBUG("%p", client);
        // Move to ex client list
        Lock lock(mutex);
        clients.remove(client, 0);
        ex_clients.push(client, 0);
    }

    void tidy_clients(Clients &client_list)
    {
        PO_DEBUG("");
        while (client_list.size(mutex))
        {
            Client *client = client_list.pop(mutex);
            PO_DEBUG("client=%p", client);
            client->stop();
            client->kill();
            delete client;
        }
        PO_DEBUG("done");
    }

    void run()
    {
        PO_DEBUG("");
        listen(sock, 10);

        Semaphore *sem = Semaphore::create();

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
            client->start(new _Socket(fd), sem);
            // Wait for the client's thread to start
            clients.push(client, mutex);
            sem->wait();
            tidy_clients(ex_clients);
        }

        delete sem;
    }

    void join()
    {
        PO_DEBUG("");
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
    ss.join();
}

}   //  namespace panglos

//  FIN
