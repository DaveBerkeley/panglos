
    /*
     *
     */

#include <stdlib.h>

#include "panglos/debug.h"
#include "panglos/mutex.h"
#include "panglos/list.h"
#include "panglos/socket.h"

#include "panglos/tx_net.h"

namespace panglos {

class XClient : public Client
{
    virtual void run() override
    {
        PO_DEBUG("");
        ASSERT(sock);

        while (true)
        {
            uint8_t buff[32];
            const int n = sock->recv(buff, sizeof(buff));

            if (n == -1)
            {
                break;
            }
            // drop incoming data ...
        }
    }

public:
    XClient(SocketServer *ss)
    :   Client(ss)
    {
        PO_DEBUG("");
    }

    ~XClient()
    {
        PO_DEBUG("");
    }

    int send(const uint8_t *data, size_t len)
    {
        return sock->send(data, len);
    }

    static XClient **get_next(XClient *c) { return (XClient **) Client::get_next(c); }
};

    /*
     *
     */

class XFactory : public TxFactory
{
    List<XClient*> clients;
    SocketServer *ss;
    Mutex *mutex;

    void add_client(Client *client)
    { 
        ASSERT(ss);
        clients.push((XClient*) client, mutex);
        ss->add_client(client);
    }

    void del_client(Client *client)
    {
        ASSERT(ss);
        clients.remove((XClient*) client, mutex);
        ss->del_client(client);
    }

    void kill()
    {
        ASSERT(ss);
        ss->kill();
    }

    class ProxySocketServer : public SocketServer
    {
        XFactory *factory;
        virtual void add_client(Client *c) override { factory->add_client(c); }
        virtual void del_client(Client *c) override { factory->del_client(c); }
        virtual void kill() override { factory->kill(); }
    public:
        ProxySocketServer(XFactory *f) : factory(f) { }
    };

    ProxySocketServer proxy;

    struct TX
    {
        const uint8_t *data;
        size_t len;
    };

    static int send_cb(XClient *client, void *arg)
    {
        ASSERT(client);
        ASSERT(arg);
        struct TX *tx = (struct TX*) arg;
        client->send(tx->data, tx->len);
        return 0;
    }

    int send(const uint8_t *data, size_t len)
    {
        PO_DEBUG("");

        struct TX tx { .data = data, .len = len, };
        clients.visit(send_cb, & tx, mutex);

        return (int) len;
    }

    class ProxySocket : public Socket
    {
        XFactory *factory;

        virtual int send(const uint8_t *data, size_t len) override
        {
            return factory->send(data, len);
        }

        virtual int recv(uint8_t *data, size_t len) override
        {
            PO_DEBUG("");
            IGNORE(data);
            return (int) len;
        }
    public:
        ProxySocket(XFactory *f) : factory(f) { }
    };

    ProxySocket psock;

    virtual Client *create_client(SocketServer *_ss) override
    {
        ss = _ss;
        return new XClient(& proxy);
    }

public:
    XFactory()
    :   clients(XClient::get_next),
        ss(0),
        mutex(0),
        proxy(this),
        psock(this)
    {
        mutex = Mutex::create(Mutex::RECURSIVE);
    }

    ~XFactory()
    {
        delete mutex;
    }

    virtual Socket *get_socket() override
    {
        return & psock;
    }
};

TxFactory *TxFactory::create()
{
    return new XFactory;
}

}   //  namespace panglos

//  FIN
