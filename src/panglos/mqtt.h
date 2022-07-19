
    /*
     *
     */

#if !defined(__PANGLOS_MQTT__)
#define __PANGLOS_MQTT__

#include "panglos/list.h"

namespace panglos {

class MqttEvent;

    /*
     *
     */

typedef struct MqttSub
{
    const char *topic;
    void (*handler)(void *arg, const char *data, int len);
    void *arg;
    struct MqttSub *next;
}   MqttSub;

    /*
     *
     */

class MqttClient
{
private:
    class Handle;
    Handle *handle;

    bool connected;

    MqttSub *next;
    panglos::List<MqttSub *> subs;

    void subscribe(MqttSub *sub);
    MqttSub *find(const char *topic, int len);

public:
    MqttClient();

    void add(MqttSub *sub);
    virtual void on_connected(MqttEvent *event);
    virtual void on_subscribed(MqttEvent *event);
    virtual void on_data(MqttEvent *event);
    virtual void on_disconnected(MqttEvent *event);
    virtual void on_unsubscribed(MqttEvent *event);
    virtual void on_error(MqttEvent *event);

    int publish(const char *topic, const char *data, int len, int qos=0, int retain=0);

    void app_start(const char *uri);
};

}   //  namespace panglos

#endif  //  __PANGLOS_MQTT__

//  FIN
