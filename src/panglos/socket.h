    /*
     *
     */

#pragma once

namespace panglos {

class Socket
{
public:
    virtual ~Socket() { }

    virtual int send(const uint8_t *data, size_t len) = 0;
    virtual int recv(uint8_t *data, size_t len) = 0;

    // TODO : called by network manager on wifi up/down events
    virtual void on_net_disconnect() { }
    virtual void on_net_connect() { }

    typedef enum { CLIENT, SERVER } Role;

    static Socket *open_udp(const char *ip, const char *port, Role role);
    static Socket *open_tcpip(const char *ip, const char *port, Role role);
};

    /*
     *
     */

class Client;

class SocketServer
{
public:
    virtual ~SocketServer() {}
    virtual void add_client(Client *) = 0;
    virtual void del_client(Client *) = 0;
    virtual void kill() = 0;
};

    /*
     *
     */

class Thread;

class Client
{
protected:
    Thread *thread;
    Socket *sock;
    SocketServer *ss;
    char *name;
    Client *next;
public:

    Client(SocketServer *ss);
    virtual ~Client();

    void start(Socket *s);
    void stop();

    virtual void run() = 0;    
    void runner();

    static void runner(void *arg);
    static Client **get_next(Client *c);

    class Factory
    {
    public:
        virtual Client *create_client(SocketServer *ss) = 0;
    };
};

    /*
     *
     */

void run_socket_server(Socket *, Client::Factory *cf);

}   //  namespace panglos

//  FIN
