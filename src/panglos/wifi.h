
#pragma once

#if defined(ARCH_LINUX)
#include <sys/socket.h>
#include <netinet/in.h>
#else
#include "lwip/sockets.h"
#endif

#include "panglos/list.h"

namespace panglos {

class WiFiInterface
{
public:

    typedef struct ip_addr {
        union {
            struct sockaddr_in v4;
            struct sockaddr_in6 v6;
        };
        const char *tostr(char *buff, size_t s);
    }   ip_addr;

    typedef struct {
        ip_addr ip;
    }   Connection;

    class Notify
    {
    public:
        virtual void on_disconnect(Connection*) = 0;
        virtual void on_connect(Connection*) = 0;
    };

    virtual void connect(Notify *s, const char *ssid, const char *pw) = 0;
    virtual void disconnect(Notify *s) = 0;

    static WiFiInterface* create();
};

    /*
     *
     */

class Semaphore;
class Mutex;

class WiFi : public WiFiInterface::Notify
{
    struct AP {
        const char *ssid;
        const char *pw;
        struct AP *next;
        static struct AP **get_next(struct AP *s) { return & s->next; }
        static int match(struct AP *s, void *);
    };

    WiFiInterface *iface;
    List<struct AP*> aps;
    Semaphore *semaphore;
    Mutex *mutex;
    bool connected;

protected:
    virtual void on_connect(WiFiInterface::Connection*) override;
    virtual void on_disconnect(WiFiInterface::Connection*) override;

public:
    WiFi(WiFiInterface *wifi);
    ~WiFi();

    void add_ap(const char *ssid, const char *pw);
    void del_aps();

    bool connect();
    bool disconnect();
    bool is_connected();
};

}   //  namespace panglos

//  FIN
