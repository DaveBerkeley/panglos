
#include <gtest/gtest.h>

#include "panglos/debug.h"
#include "panglos/thread.h"
#include "panglos/list.h"
#include "panglos/time.h"
#include "panglos/socket.h"

using namespace panglos;

    /*
     *
     */

class EchoClient : public Client
{
     virtual void run() override
     {
         PO_DEBUG("");
     }
public:
    EchoClient(SocketServer *ss)
    :   Client(ss)
    {
    }
};

class TestFactory : public Client::Factory
{
    virtual Client *create_client(SocketServer *ss) override
    {
        Client *client = new EchoClient(ss);
        PO_DEBUG("%p", client);

        static int count = 0;
        count += 1;
        if (count == 3)
        {
            ss->kill();
        }

        return client;
    }

public:
    static void run(void *arg)
    {
        ASSERT(arg);
        Socket *s = (Socket*) arg;
        TestFactory factory;
        run_socket_server(s, & factory);
    }
};

static Thread *run_server(Socket *s)
{
    Thread *thread = Thread::create("server");
    thread->start(TestFactory::run, s);
    return thread;
}

struct SocketAddr
{
    const char *ip;
    const char *port;
};

static void run_client(void *arg)
{
    ASSERT(arg);
    struct SocketAddr *sa = (struct SocketAddr *) arg;

    Time::sleep(1);

    panglos::Socket *socket = panglos::Socket::open_tcpip(sa->ip, sa->port, panglos::Socket::CLIENT);

    while (true)
    {
        const char *msg = "hello";
        const int n = socket->send((uint8_t*) msg, strlen(msg));
        PO_DEBUG("n=%d", n);
        break;
    }

    delete socket;
}

    /*
     *
     */

TEST(Socket, Test)
{
    struct SocketAddr sa = {
        .ip = "localhost",
        .port = "6667",
    };

    Socket *s = Socket::open_tcpip(sa.ip, sa.port, Socket::SERVER);
    Thread *thread = run_server(s);

    const int num = 3;
    Thread *threads[num];

    for (int i = 0; i < 3; i++)
    {
        threads[i] = Thread::create("client");
        threads[i]->start(run_client, & sa);
    }

    for (int i = 0; i < 3; i++)
    {
        threads[i]->join();
        delete threads[i];
    }

    thread->join();
    delete thread;
    delete s;
}

//  FIN
