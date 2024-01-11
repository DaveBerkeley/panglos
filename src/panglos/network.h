
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
     *
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
     *
     */

class Connection;

class Interface
{
    Interface *next;
public:
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

public:
    Interface(const char *_name);
    virtual ~Interface();

    virtual bool is_connected(IpAddr *ipaddr=0) = 0;

    void add_connection(Connection *con);
    void del_connection(Connection *con);

    void on_connect();
    void on_disconnect();

    static Interface **get_next(Interface *iface) { return & iface->next; }

    static int match_name(Interface *iface, void *arg);
};

    /*
     *
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

    virtual void connect() = 0;
    virtual void disconnect() = 0;

    static WiFiInterface *create();
};

}   //  namespace panglos

//  FIN

