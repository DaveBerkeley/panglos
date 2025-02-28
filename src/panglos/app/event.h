
    /*
     *  Application specific Event/Queue
     */

#pragma once 

#include "panglos/drivers/uart.h"

namespace panglos {

class Event 
{
public:
    enum Type {
        IDLE,
        INIT,
        STOP,

        KEY,
        UART,
        MQTT,
        DISPLAY,
        DATE_TIME,
        LOCATION,

        USER,
    };

    enum Type type;

    union payload {
        struct {
            const char *text;
            int len;
        }   mqtt;
        struct {
            char text[16];
        }   display;
        struct dt {
            enum DT_SOURCE { UNKNOWN=0, RTC, GPS, NET, };

            uint16_t yy;
            uint8_t mm;
            uint8_t dd;
            uint8_t h;
            uint8_t m;
            uint8_t s;
            enum DT_SOURCE source;
        }   dt;
        struct {
            float lat;
            float lon;
            float alt;
        }   location;
        struct key {
            enum Code { ON, OFF, PRESS, HOLD };
            enum Code code;
            void *arg;
        }   key;
        // TODO : add any fields that are needed ...
        struct vp {
            void *arg;
        }   vp;
        struct u32 {
            uint32_t d;
        }   u32;
        struct u64 {
            uint64_t d;
        }   u64;
    }   payload;

    // Wrap a standard Queue
    class Queue {
    public:
        virtual ~Queue() { }
    };

    static Queue *create_queue(int num, Mutex *mutex);
    bool get(Queue *queue, int timeout);
    bool put(Queue *queue);
    int queued(Queue *queue);
    void stop(Queue *queue);

    static const LUT type_lut[];
};

    /*
     *  Linked list of event handlers
     */

struct EventHandler
{
    enum Event::Type type;
    bool (*fn)(void *arg, Event *event, Event::Queue *q);
    void *arg;
    struct EventHandler *next;
    static struct EventHandler **get_next(struct EventHandler *d) { return & d->next; }
 
    static void add_handler(enum Event::Type type, bool (*fn)(void *arg, Event *event, Event::Queue *q), void *arg);
    static bool del_handler(struct EventHandler *eh);

    static struct EventHandler *handle_event(Event::Queue *queue, Event *event);
    static void unlink_handlers(Event::Type type);

    static enum Event::Type get_user_code();
};


    /*
     *  UART events
     */

struct UartHandler
{
    Event::Queue *queue;
    UART *uart;
    void *arg;
};

void on_uart(In::Event* ev, void *arg);

}   //  namespace panglos

//  FIN
