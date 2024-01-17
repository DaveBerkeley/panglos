
#include <gtest/gtest.h>

#include "panglos/debug.h"

#include "panglos/mutex.h"
#include "panglos/semaphore.h"
#include "panglos/list.h"
#include "panglos/batch.h"
#include "panglos/object.h"
#include "panglos/event_queue.h"

#include "panglos/network.h"

using namespace panglos;

    /*
     *
     */

class TestWiFi;

class ConnectJob : public BatchTask::Job
{
    TestWiFi *wifi;
    virtual void run() override;
public:
    ConnectJob(TestWiFi *w) : wifi(w) { }
};

class DisconnectJob : public BatchTask::Job
{
    TestWiFi *wifi;
    virtual void run() override;
public:
    DisconnectJob(TestWiFi *w) : wifi(w) { }
};

    /*
     *
     */

class TestWiFi : public WiFiInterface
{
    BatchTask *batch;
    const char *ap;
    ConnectJob connect_job;
    DisconnectJob disconnect_job;
public:
    bool connected;

    virtual bool is_connected(State *) override
    {
        PO_DEBUG("");
        return connected;
    }

    virtual void connect() override
    {
        PO_DEBUG("");
        batch->execute(& connect_job);
    }

    virtual void disconnect() override
    {
        PO_DEBUG("");
        batch->execute(& disconnect_job);
    }

    TestWiFi(const char *name, BatchTask *b, const char *_ap)
    :   WiFiInterface(name),
        batch(b),
        ap(_ap),
        connect_job(this),
        disconnect_job(this)
    {
        PO_DEBUG("%s", ap);
    }
};

    /*
     *
     */

void ConnectJob::run()
{
    PO_DEBUG("");
    wifi->connected = true;
    wifi->on_connect();
}

void DisconnectJob::run()
{
    PO_DEBUG("");
    wifi->connected = false;
    wifi->on_disconnect();
}

    /*
     *
     */

class TestConnection : public Connection
{
public:
    Interface *iface;

    virtual void on_connect(Interface *_iface) override
    {
        PO_DEBUG("");
        iface = _iface;
    }
    virtual void on_connect_fail(Interface *) override
    {
        PO_DEBUG("");
    }
    virtual void on_disconnect(Interface *) override
    {
        PO_DEBUG("");
        iface = 0;
    }

    TestConnection()
    :   iface(0)
    {
    }
};

    /*
     *
     */

TEST(Net, WiFiAp)
{
    TestWiFi wifi("wifi", 0, 0);

    wifi.add_ap("hello", "world");
    wifi.add_ap("one", "more");

    wifi.del_ap("hello");
}

    /*
     *
     */

static void wait_job(BatchTask *batch)
{
    BatchTask::WaitJob job;
    batch->execute(& job);
    job.wait();
}

    /*
     *
     */

TEST(Net, Connect)
{
    Objects::objects = Objects::create();
    BatchTask *batch = BatchTask::start();
    TestWiFi wifi("wifi", batch, 0);

    // simulate multiple connections
    const int num = 4;
    TestConnection con[num];
    for (int i = 0; i < num; i++)
    {
        EXPECT_FALSE(con[i].iface);
        wifi.add_connection(& con[i]);
    }

    wifi.connect();
    wait_job(batch);
 
    for (int i = 0; i < num; i++)
    {
        EXPECT_EQ(& wifi, con[i].iface);
    }

    wifi.disconnect();
    wait_job(batch);

    for (int i = 0; i < num; i++)
    {
        EXPECT_FALSE(con[i].iface);
    }

    delete batch;
    delete Objects::objects;
}

    /*
     *
     */

TEST(Net, Network)
{
    TestWiFi wifi("wifi", 0, 0);

    Network net;
    net.add_interface(& wifi);

    Interface *iface;

    // any | the interface
    iface = net.get_interface();
    EXPECT_EQ(iface, & wifi);

    iface = net.get_interface("wrong");
    EXPECT_FALSE(iface);

    iface = net.get_interface("wifi");
    EXPECT_EQ(iface, & wifi);
}

TEST(Net, GetIface)
{
    TestWiFi a("wifi", 0, 0);
    TestWiFi b("bbb", 0, 0);
    TestWiFi c("ccc", 0, 0);

    Network net;
    net.add_interface(& a);
    net.add_interface(& b);
    net.add_interface(& c);

    Interface *iface;

    Network::Iterator iter;

    iface = net.get_interface(& iter);
    EXPECT_EQ(iface, & a);
    iface = net.get_interface(& iter);
    EXPECT_EQ(iface, & b);
    iface = net.get_interface(& iter);
    EXPECT_EQ(iface, & c);

    iface = net.get_interface(& iter);
    EXPECT_FALSE(iface);
}

    /*
     *
     */

class NetManager : public Connection
{
    Network *net;
    EvQueue *queue;
    BatchTask *batch;

    class Job : public BatchTask::Job
    {
        NetManager *nm;

        virtual void run() override
        {
            PO_DEBUG("");
            nm->try_connect();
        }
    public:
        Job(NetManager *m) : nm(m) { }
    };

    Job job;

    // Use the EvQueue to schedule the connection attempt
    class Event : public EvQueue::Event
    {
        NetManager *nm;

        virtual void run(EvQueue *) override
        {
            PO_DEBUG("");
            nm->batch->execute(& nm->job);
        }

    public:
        Event(NetManager *n) : nm(n) { }
    };

    Event event;

    virtual void on_connect(Interface *) override
    {
        PO_DEBUG("TODO");
    }

    virtual void on_connect_fail(Interface *) override
    {
        // can get multiple connect_fail events from a single connect attempt
        // ie one per registered AP 
        PO_DEBUG("TODO");
    }

    virtual void on_disconnect(Interface *) override
    {
        PO_DEBUG("TODO");
        // TODO : schedule reconnect attempts
        event.when = 10;
        queue->add(& event);
    }

    void try_connect()
    {
        PO_DEBUG("TODO");
        //net->connect();
    }

public:
    NetManager(Network *n, EvQueue *q, BatchTask *b)
    :   net(n),
        queue(q),
        batch(b),
        job(this),
        event(this)
    {
        ASSERT(net);
        ASSERT(queue);
    }
};

TEST(Net, Manager)
{
    Objects::objects = Objects::create();
    BatchTask *batch = BatchTask::start();
    Network net;
    EvQueue eq;
    NetManager man(& net, & eq, batch);
    TestWiFi wifi("wifi", batch, "middle");

    wifi.add_ap("hello", "world");
    wifi.add_ap("middle", "middle");
    wifi.add_ap("last", "one");

    wifi.add_connection(& man);
    net.add_interface(& wifi);

    wifi.connect();
    // cause asynchronous disconnect
    // build mechanism to schedule a reconnect batch job
    wifi.disconnect();

    for (Time::tick_t i = 0; i < 100; i++)
    {
        eq.run(i);
        // Wait for batch jobs to be processed
        wait_job(batch);
    }
 
    delete batch;
    delete Objects::objects;
}

//  FIN
