
    /*
     *
     */

#if !defined(__PANGLOS_MQTT__)
#define __PANGLOS_MQTT__

#include "panglos/list.h"
#include "panglos/io.h"

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
    static MqttSub **get_next(MqttSub *sub) { return & sub->next; }

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

    /*
     *
     */

class MqttOut : public Out
{
    MqttClient *mqtt;
    const char *topic;

public:
    MqttOut(MqttClient *_mqtt, const char *_topic)
    :   mqtt(_mqtt),
        topic(_topic)
    {
    }

    virtual int tx(const char* data, int n) override
    {
        mqtt->publish(topic, data, n);
        return n;
    }
};

}   //  namespace panglos

#endif  //  __PANGLOS_MQTT__

//  FIN
