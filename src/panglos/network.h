
#pragma once

#if defined(ARCH_LINUX)
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#else
#include "lwip/sockets.h"
#endif

#include "panglos/list.h"

namespace panglos {

    /*
     *  AccessPoint simply holds the ssid/pw credentials for a known AP
     */

class AccessPoint
{
    AccessPoint *next;
public:
    const char *ssid;
    const char *pw;

    AccessPoint(const char *_ssid, const char *_pw);
    ~AccessPoint();

    static AccessPoint **get_next(AccessPoint *info) { return & info->next; }

    static int match_ssid(AccessPoint *ap, void *arg);
};

    /*
     *  An interface is a device that can have an ip address
     */

class Connection;
class WiFiInterface;

class Interface
{
    Interface *next;
    const char *name;
protected:
    panglos::Mutex *con_mutex;
    List<Connection*> connections;

public:
    typedef struct IpAddr {
        union {
            struct sockaddr_in v4;
            struct sockaddr_in6 v6;
        };
        const char *tostr(char *buff, size_t s);
    }   IpAddr;

    typedef struct State {
        IpAddr  ip;
        IpAddr  gw;
        IpAddr  mask;
    }   State; 

public:
    Interface(const char *name);
    virtual ~Interface();

    virtual bool is_connected(State *state=0) = 0;
    virtual WiFiInterface *get_wifi() { return 0; }

    void add_connection(Connection *con);
    void del_connection(Connection *con);

    void on_connect();
    void on_disconnect();

    virtual void connect() = 0;
    virtual void disconnect() = 0;

    static Interface **get_next(Interface *iface) { return & iface->next; }

    static int match_name(Interface *iface, void *arg);
    const char *get_name() { return name; }
};

    /*
     *  Connection is an observer of an Interface
     */

class Connection
{
    Connection *next;
public:

    virtual void on_connect(Interface *) = 0;
    virtual void on_disconnect(Interface *) = 0;

    static Connection **get_next(Connection *con) { return & con->next; }
};

    /*
     *  Helper class to wait for a connection
     */

class Semaphore;

class ConnectionWaiter : public Connection
{
    Semaphore *sem;
    Interface *iface;

    virtual void on_connect(Interface *i) override;
    virtual void on_disconnect(Interface *i) override;

public:
    ConnectionWaiter();
    ~ConnectionWaiter();

    bool wait();
};

    /*
     *
     */

class Network
{
    panglos::Mutex *mutex;
    List<Interface*> interfaces;
public:
    Network(panglos::Mutex *m=0);
    ~Network();

    void add_interface(Interface *iface);
    Interface *get_interface(const char *name=0);

    struct Iterator
    {
        Interface *iface;
        Iterator() : iface(0) { }
    };

    Interface *get_interface(struct Iterator *);

    static void start_mdns(const char *name);
};

    /*
     *
     */

class WiFiInterface : public Interface
{
protected:
    List<AccessPoint*> access_points;
    panglos::Mutex *ap_mutex;

public:
    WiFiInterface(const char *name);
    ~WiFiInterface();

    void add_ap(const char *ssid, const char *pw);
    void del_ap(const char *ssid);

    struct ApIter {
        AccessPoint *ap;
        ApIter() : ap(0) { }
    };

    AccessPoint *get_ap(struct ApIter *iter);
    const char *get_ssid();

    static WiFiInterface *create();
};

}   //  namespace panglos

//  FIN

