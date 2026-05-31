
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
    int idx;

    virtual void run() override
    {
        PO_DEBUG("");
        if (idx % 2)
        {
            // half the threads wait
            Time::msleep(100);
        }
    }
public:
    EchoClient(SocketServer *ss, int n)
    :   Client(ss),
        idx(n)
    {
    }
};

class TestFactory : public Client::Factory
{
    Socket *socket;
    int num;

    virtual Client *create_client(SocketServer *ss) override
    {
        static int count = 0;
        Client *client = new EchoClient(ss, count);
        PO_DEBUG("%p", client);

        count += 1;
        if (count == num)
        {
            ss->kill();
        }

        return client;
    }

public:
    TestFactory(Socket *s, int n)
    :   socket(s),
        num(n)
    {
    }

    static void run(void *arg)
    {
        ASSERT(arg);
        TestFactory *factory = (TestFactory *) arg;
        run_socket_server(factory->socket, factory);
    }
};

static Thread *run_server(Socket *s, int num)
{
    static TestFactory factory(s, num);
    Thread *thread = Thread::create("server");
    thread->start(factory.run, & factory);
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

    const int num = 4;

    Socket *s = Socket::open_tcpip(sa.ip, sa.port, Socket::SERVER);
    Thread *thread = run_server(s, num);

    Thread *threads[num];

    for (int i = 0; i < num; i++)
    {
        threads[i] = Thread::create("client");
        threads[i]->start(run_client, & sa);
    }

    for (int i = 0; i < num; i++)
    {
        threads[i]->join();
        delete threads[i];
    }

    thread->join();
    delete thread;
    delete s;
}

//  FIN
