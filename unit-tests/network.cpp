
#include <gtest/gtest.h>

#include "panglos/debug.h"

#include "panglos/mutex.h"
#include "panglos/semaphore.h"
#include "panglos/list.h"
#include "panglos/batch.h"
#include "panglos/object.h"

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
    ConnectJob connect_job;
    DisconnectJob disconnect_job;
public:
    bool connected;

    virtual bool is_connected(IpAddr *) override
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

    TestWiFi(const char *name, BatchTask *b)
    :   WiFiInterface(name),
        batch(b),
        connect_job(this),
        disconnect_job(this)
    {
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
    TestWiFi wifi("wifi", 0);

    wifi.add_ap("hello", "world");
    wifi.add_ap("one", "more");

    wifi.del_ap("hello");
}

    /*
     *
     */

class WaitJob : public BatchTask::Job
{
    Semaphore *semaphore;
public:
    WaitJob()
    :   semaphore(0)
    {
        semaphore = Semaphore::create();
    }
    ~WaitJob()
    {
        delete semaphore;
    }
    virtual void run() override
    {
        semaphore->post();
    }

    void wait()
    {
        semaphore->wait();
    }
};

static void wait_job(BatchTask *batch)
{
    WaitJob job;
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
    TestWiFi wifi("wifi", batch);

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
    TestWiFi wifi("wifi", 0);

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

//  FIN
