
#pragma once

#include "panglos/list.h"

namespace panglos {

class Network
{
public:
    class Interface
    {
    public:
        const char *ip;
        Interface *next;
        static Interface **get_next(Interface *interface) { return & interface->next; }

        Interface(const char *s) : ip(s) { }
    };

    static panglos::List<Interface*> interfaces;

    static void add_interface(const char *ip);
    static void del_interface(const char *ip);

    class Connect
    {
    public:
        Connect *next;

        virtual ~Connect() {}
        virtual void on_connect() = 0;
        virtual void on_disconnect() = 0;
        static Connect **get_next(Connect *c) { return & c->next; }
    };

    static panglos::List<Connect*> connects;

    static void add_connect(Connect *connect);
    static void del_connect(Connect *connect);

    static void start_mdns(const char *hostname);
};

}   //  namespace panglos

//  FIN
